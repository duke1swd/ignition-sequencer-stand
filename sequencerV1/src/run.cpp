/*
 * This code handles sequencing for engine operations.
 * Two modes, ignition test, and full burn
 *
 * States used in this module
 * 	runStart
 * 		This state is purely time driven.  It turns on the spark, IPA,
 * 		and N2O at the right time.  Once all three are on we switch to
 * 		runIgPressCheck.
 *
 * 		Note: exit from this state shuts down the igniter.  If we exit
 * 		to a state that still wants the igniter on then that state's
 * 		start routine will turn it back on.  The state machine makes
 * 		sure there are no output glitches as a result of this technique.
 *
 * 	runIgPress
 * 		Run the igniter until eitther too much time has past (error)
 * 		or the igniter chamber pressure comes up to nominal.
 *
 * 	runIgRun
 * 		This state runs for a fixed period of time.  Somewhere along
 * 		the way it turns off the spark.  If the igniter is still
 * 		at pressure at the end of the time we either transition to motor
 * 		run or we are done with the igniter test.
 *
 * 	igRunReport
 * 		This state displays a test report on the screen until any button
 * 		is pressed.  This is the normal exit from runIgRun when not
 * 		exiting to main motor run state.
 *
 * 	runIgDebug
 * 		This state runs the entire igniter sequence on a fixed set
 * 		of timings.
 *
 * 	shutdown
 * 		Turns off ig and main valves, waits a while, then exits
 * 		This is the normal exit from runIgDebug.
 *
 * 	igThisTest
 * 		This variable contains a pointer to the test entry state.
 * 		Ignition tests eventually exit to this state, saving on
 * 		menu clicking when repeated igntion tests are being run.
 */

#define	NO_SENSOR 1

#include "parameters.h"
#include "state_machine.h"
#include "errors.h"
#include "joystick.h"
#include "tft_menu.h"
#include "io_ref.h"
#include "events.h"
#include "mainvalves.h"
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library

#define	DAQ1PRESSURE	1    // put state of pressure sensor on daq1 line.

extern void spark_run();
extern Adafruit_ST7735 tft;
extern long spark_bias;

/*
 * Report variables
 */
long rep_n_samples;
long rep_max_pressure;
long rep_sum_pressure;
bool rep_stop_good;

void runStartEnter();
void runIgExit();
const struct state *runStartCheck();
void runIgEnter();
const struct state *runIgPressCheck();
struct state runStart = { "runStart", &runStartEnter, &runIgExit, &runStartCheck};
struct state runIgPress = { "runIgPress", &runIgEnter, &runIgExit, &runIgPressCheck};
const struct state *runIgRunCheck();
struct state runIgRun = { "runIgRun", &runIgEnter, &runIgExit, &runIgRunCheck};
const struct state *runIgDebugCheck();
struct state runIgDebug = {"runIgDebug", &runStartEnter, &runIgExit, &runIgDebugCheck};
void igReportEnter();
const struct state *igRepCheck();
struct state igRunReport { "igRunRep", &igReportEnter, NULL, igRepCheck};
void shutdownEnter();
const struct state *shutdownCheck();
struct state shutdown { "shutdown", &shutdownEnter, NULL, shutdownCheck};

static bool safe_ok()
{
	// Igniter must not be safe, and
	// Ignition tests require main to be safe.
	return i_safe_ig->current_val == 0 && i_safe_main->current_val == 1;
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
	spark_bias = test_start_t;
	rep_n_samples = 0;
	rep_max_pressure = 0;
	rep_sum_pressure = 0;
	rep_stop_good = false;

	at_pressure_t = 0;
	runMainExit();
	o_daq0->cur_state = on;	// goes on at commanded start.  runMainExit sets this to zero
}

/*
 * On exit make sure both valves are closed (off)
 * Kill the signal to the spark generator
 *
 * Called on all exits
 * 
 * Saves the DAQ output state.  This is used to persist
 * the output state should we transition to a normal (i.e. not error) state.
 */
enum output_state l_daq0;
enum output_state l_daq1;
void runIgExit()
{
	o_spark->cur_state = off;
	o_ipaIgValve->cur_state = off;
	o_n2oIgValve->cur_state = off;
	o_amberStatus->cur_state = on;
	o_greenStatus->cur_state = off;

	// save daq state, but turn daq state off in case of error exit
	l_daq0 = o_daq0->cur_state;
	l_daq1 = o_daq1->cur_state;
	o_daq0->cur_state = off;
	o_daq1->cur_state = off;
}

/*
 * check for abort conditions
 */
static const struct state * allAborts()
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

	if (!SENSOR_SANE(p)) {
		event(IgPressFail);
		return error_state(errorIgPressureInsane, p);
	}

	// abort if pressure sensor broken
#ifndef NO_SENSOR
	if (p < min_pressure) {
		event(IgPressFail);
		return error_state(errorIgNoPressure, p);
	}
#endif

	// p is the filtered pressure (counts * 4)
	p = i_main_press->filter_a;

	if (!SENSOR_SANE(p)) {
		event(MainPressFail);
		return error_state(errorMainPressureInsane, p);
	}

	// abort if pressure sensor broken
#ifndef NO_SENSOR
	if (p < min_pressure) {
		event(MainPressFail);
		return error_state(errorMainNoPressure, p);
	}
#endif

	return NULL;
}


/*
 * Sequence up the igniter.
 * Then move to run runIgPressure
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
	o_daq0->cur_state = l_daq0;
	o_daq1->cur_state = l_daq1;
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
#ifdef DAQ1PRESSURE
		o_daq1->cur_state = on;		// goes on at first good pressure sense
#endif
		// record pressure for run reporting
		record_p(p);
		return &runIgRun;
	}

	// t is how long we've been in this state
	t = loop_start_t - state_enter_t;

	// If no ignition and too much time has passed, give up with an error
	if (t > ig_pressure_time)
		return error_state(errorIgNoIg, (unsigned int)t);
	
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
	else
		o_spark->cur_state = off;

	// if we are within pressure grace period keep running
	if (t <= ig_pressure_grace)
		return current_state;

	// Grace is over.

	// Keep going until either too much time has passed or we flame out
	if (p >= good_pressure && t <= ig_run_time)
		return current_state;
	if (p >= good_pressure)
		rep_stop_good = true;
	return &igRunReport;
}

/*
 * Put the report on the screen
 * Prints
 *	run time,
 *	time at first pressure
 *	ave pressure
 *	max pressure
 *	stop condition (red or green)
 */
void igReportEnter()
{
	// background is Blue
	tft.fillScreen(ST7735_BLUE);
	tft.setTextSize(TM_TXT_SIZE+1);
	tft.setCursor(8, TM_TXT_OFFSET);
	// text is white
	tft.setTextColor(ST7735_WHITE);
	tft.print(F("REPORT"));
	tft.setTextSize(TM_TXT_SIZE);
	tft.setCursor(20, 1 * TM_TXT_HEIGHT+16+TM_TXT_OFFSET);
	tft.print(F("tt: ")); tft.print(loop_start_t - test_start_t);
	tft.setCursor(20, 2 * TM_TXT_HEIGHT+16+TM_TXT_OFFSET);

	// Run time is reported in red if we stopped on flame out,
	// green if we stopped on time
	if (rep_stop_good)
		tft.setTextColor(ST7735_GREEN);
	else
		tft.setTextColor(ST7735_RED);
	tft.print(F("rt: "));
	if (at_pressure_t)
		tft.print(loop_start_t - at_pressure_t);
	else
		tft.print(F("none"));

	tft.setTextColor(ST7735_WHITE);
	tft.setCursor(20, 3 * TM_TXT_HEIGHT+16+TM_TXT_OFFSET);
	tft.print(F("mx: ")); tft.print(rep_max_pressure);
	tft.setCursor(20, 4 * TM_TXT_HEIGHT+16+TM_TXT_OFFSET);
	tft.print(F("av: ")); tft.print(rep_sum_pressure/rep_n_samples);
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

/*
 * This routine runs the fixed, 1-second run.
 */
const struct state * runIgDebugCheck()
{
	unsigned long t;
	unsigned int p;
	const struct state *es;

	// handle aborts
	es = allAborts();
	if (es)
		return es;

	// t is how long we've been in this state
	t = loop_start_t - state_enter_t;

	if (t >= ig_spark_time && t < ig_spark_off_time)
		spark_run();

	if (t >= ig_n2o_time)
		o_n2oIgValve->cur_state = on;

	if (t >= ig_ipa_time)
		o_ipaIgValve->cur_state = on;

	// p is the filtered pressure (counts * 4)
	p = i_ig_pressure->filter_a;

#ifdef DAQ1PRESSURE
	// daq 1 records if good pressure or not.
	if (p >= good_pressure)
		o_daq1->cur_state = on;
	else
		o_daq1->cur_state = off;
#endif

	// Run for a fixed length of time.
	if (t > ig_run_time)
		return &shutdown;
	
	// keep waiting
	return current_state;
}

/*
 * Shutdown state
 */
void shutdownEnter()
{
	o_ipaIgValve->cur_state = off;
	o_n2oIgValve->cur_state = off;
	mainIPAClose();
	mainN2OClose();
	o_daq0->cur_state = off;
	o_daq1->cur_state = off;
}

const struct state * shutdownCheck()
{
	unsigned long t;
	extern const struct state *igThisTest;

	t = loop_start_t - state_enter_t;

	if (t > shutdown_timeout)
		return igThisTest;
	return current_state;
}
