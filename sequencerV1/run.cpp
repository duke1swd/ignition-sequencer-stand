/*
 * This code handles sequencing for engine operations.
 * Two modes, ignition test, and full burn
 */

#include "parameters.h"
#include "state_machine.h"
#include "errors.h"
#include "joystick.h"
#include "tft_menu.h"
#include "io_ref.h"
#include "run.h"
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library

extern void mainIPAOpen();
extern void mainIPAClose();
extern void mainN2OOpen();
extern void mainN2OClose();
extern void spark_run();
extern Adafruit_ST7735 tft;

unsigned char runMode;	// ignition only?  Or whole enchilada?

/*
 * Report variables
 */
long rep_n_samples;
long rep_max_pressure;
long rep_sum_pressure;

void runStartEnter();
void runIgExit();
const struct state *runStartCheck();
void runIgEnter();
const struct state *runIgPressCheck();
struct state runStart = { "runStart", &runStartEnter, &runIgExit, &runStartCheck};
struct state runIgPress = { "runIgPress", &runIgEnter, &runIgExit, &runIgPressCheck};
const struct state *runIgRunCheck();
struct state runIgRun = { "runIgRun", &runIgEnter, &runIgExit, &runIgRunCheck};
void igReportEnter();
const struct state *igRepCheck();
struct state igRunReport { "igRunRep", &igReportEnter, NULL, igRepCheck};

void runInit(unsigned char mode)
{
	runMode = mode;
}

static bool safe_ok()
{
	int s;

	// Igniter must not be safe
	if (i_safe_ig->current_val == 1)
		return 0;

	// Ignition tests require main to be safe.
	// Main engine bur requires main to be not safe.
	s = i_safe_main->current_val;
	return (runMode == RUN_IG_ONLY)? (s == 1): (s == 0);
}

static bool power_ok()
{
	return i_power_sense->current_val == 1;
}

static void record_p(unsigned int p)
{
	if (p > rep_max_pressure)
		rep_max_pressure = p;
	rep_n_samples++;
	rep_sum_pressure += p;

	if (p >= good_pressure) {
		o_amberStatus->cur_state = off;
		o_greenStatus->cur_state = on;
	} else {
		o_amberStatus->cur_state = on;
		o_greenStatus->cur_state = off;
	}
}

/*
 * RUN START: sequence up the igniter
 */
unsigned long test_start_t;
unsigned long at_pressure_t;

/*
 * On exit make sure main valves are closed.
 * Then perform all ignition exit actions.
 */
void runMainExit()
{
	mainIPAClose();
	mainN2OClose();
	runIgExit();
}

/*
 * On entry clear the reporting variables.
 */
void runStartEnter()
{
	test_start_t = millis();
	rep_n_samples = 0;
	rep_max_pressure = 0;
	rep_sum_pressure = 0;
	at_pressure_t = 0;
	runMainExit();
	o_daq0->cur_state = on;	// goes on at commanded start
}

/*
 * On exit make sure both valves are closed (off)
 * Kill the signal to the spark generator
 *
 * Called on all exits
 */
void runIgExit()
{
	o_spark->cur_state = off;
	o_ipaIgValve->cur_state = off;
	o_n2oIgValve->cur_state = off;
	o_amberStatus->cur_state = on;
	o_greenStatus->cur_state = off;
	o_daq0->cur_state = off;
	o_daq1->cur_state = off;
}

/*
 * check for abort conditions
 */
const struct state * allAborts()
{
	unsigned int p;

	// operator abort by remote, button 2, or joystick
	if (joystick_edge_value == JOY_PRESS ||
			i_cmd_2->current_val ||
			i_push_2->current_val)
		return error_state(errorIgTestAborted);

	// operator abort by safe switch
	if (!safe_ok())
		return error_state(errorIgTestSafe);

	// operator abort by power switch
	if (!power_ok())
		return error_state(errorIgTestPower);

	// p is the filtered pressure (counts * 4)
	p = i_ig_pressure->filter_a;

	if (!SENSOR_SANE(p))
		return error_state(errorPressureInsane);

	// abort if pressure sensor broken
	if (p < min_pressure)
		return error_state(errorIgNoPressure);

	return NULL;
}


/*
 * Sequence up the igniter.
 * The move to run runIgPressure
 */
const struct state * runStartCheck()
{
	unsigned long t;
	unsigned char conditions;
	const struct state *es;

	// handle aborts
	es = allAborts();
	if (es)
		return es;

	// t is how long we've been in this state
	// On first iteration loop_start_t might be behind test_start_t
	if (loop_start_t > test_start_t)
		t = loop_start_t - test_start_t;
	else
		t = 0;

	// job of this state is to set 3 conditions at the right time.
	conditions = 0;
	if (t >= ig_spark_time) {
		spark_run();
		conditions++;
	}
	if (t >= ig_n2o_time) {
		o_n2oIgValve->cur_state = on;
		conditions++;
	}
	if (t >= ig_ipa_time) {
		o_ipaIgValve->cur_state = on;
		conditions++;
	}

	// once all three conditions are met, we are done
	if (conditions >= 3)
		return &runIgPress;

	return current_state;
}

void runIgEnter()
{
	o_n2oIgValve->cur_state = on;
	o_ipaIgValve->cur_state = on;
}

/*
 * Wait for the igniter to come up to pressure.
 * the move to run runIgRun
 */
const struct state * runIgPressCheck()
{
	unsigned long t;
	unsigned int p;
	const struct state *es;

	// handle aborts
	es = allAborts();
	if (es)
		return es;

	// spark is running entire time we are waiting for pressure
	spark_run();

	// p is the filtered pressure (counts * 4)
	p = i_ig_pressure->filter_a;

	// If good pressure, e.g. ignition, record the pressure sample
	// and exit to the runIgRun state.
	if (p >= good_pressure) {
		// record when we first came up to pressure
		at_pressure_t = loop_start_t;
		o_daq1->cur_state = on;		// goes on at first good pressure sense
		// record pressure for run reporting
		record_p(p);
		return &runIgRun;
	}

	// t is how long we've been in this state
	t = loop_start_t - state_enter_t;

	// If no ignition and too much time has passed, give up with an error
	if (t > ig_pressure_time)
		return error_state(errorIgNoIg);
	
	// keep waiting
	return current_state;
}

/*
 * Running The Igniter
 * Wait for stable burn, turn off spark, various exits.
 */
const struct state * runIgRunCheck()
{
	unsigned long t;
	unsigned int p;
	const struct state *es;

	// handle aborts
	es = allAborts();
	if (es)
		return es;

	// p is the filtered pressure (counts * 4)
	p = i_ig_pressure->filter_a;
	record_p(p);

	// t is how long since pressure came up
	t = loop_start_t - at_pressure_t;

	// keep spark going for awhile after pressure comes up
	if (t <= ig_spark_cont_time)
		spark_run();

	// if we are within pressure grace period keep running
	if (t <= ig_pressure_grace)
		return current_state;

	// Grace is over.

	if (runMode == RUN_IG_ONLY) {
		// Keep going until either too much time has passed or we flame out
		if (p >= good_pressure && t <= ig_run_time)
			return current_state;
		return &igRunReport;
	} else {
		if (p < good_pressure)
			return error_state(errorIgFlameOut);
		// XXX QQQ XXX
		// transition to running the main engine
		return &igRunReport;	// replace this with the main engine state
	}
}

/*
 * Put the report on the screen
 * Prints
 *	run time,
 *	time at first pressure
 *	ave pressure
 *	max pressure
 */
void igReportEnter()
{
	// background is Blue
	tft.fillScreen(ST7735_BLUE);
	tft.setTextSize(TM_TXT_SIZE+1);
	tft.setCursor(8, TM_TXT_OFFSET);
	// text is white
	tft.setTextColor(ST7735_WHITE);
	tft.print("REPORT");
	tft.setTextSize(TM_TXT_SIZE);
	tft.setCursor(20, 1 * TM_TXT_HEIGHT+16+TM_TXT_OFFSET);
	tft.print(loop_start_t - test_start_t);
	tft.setCursor(20, 2 * TM_TXT_HEIGHT+16+TM_TXT_OFFSET);
	if (at_pressure_t)
		tft.print(loop_start_t - at_pressure_t);
	else
		tft.print("none");
	tft.setCursor(20, 3 * TM_TXT_HEIGHT+16+TM_TXT_OFFSET);
	tft.print(rep_max_pressure);
	tft.setCursor(20, 4 * TM_TXT_HEIGHT+16+TM_TXT_OFFSET);
	tft.print(rep_sum_pressure/rep_n_samples);
}

/*
 * This routine figures out when to stop displaying the status report.
 */

const struct state * igRepCheck()
{
	extern const struct state *igThisTest;

	if (joystick_edge_value == JOY_PRESS)
		return igThisTest;
	return &igRunReport;
}
