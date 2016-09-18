/*
 *  This code handles the ignition system test
 */

#include "parameters.h"
#include "state_machine.h"
#include "errors.h"
#include "joystick.h"
#include "tft_menu.h"
#include "io_ref.h"
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library

extern Adafruit_ST7735 tft;
extern struct menu main_menu;

void igTestEnter();
void igTestExit();
void igLRTestEnter();
const struct state *igLocalTestEntryCheck();
const struct state *igRemoteTestEntryCheck();
const struct state *igTestCheck();
struct state igLocalTestEntry = { "igLocalTest", &igLRTestEnter, NULL, &igLocalTestEntryCheck};
struct state igRemoteTestEntry = { "igRemoteTest", &igLRTestEnter, NULL, &igRemoteTestEntryCheck};
struct state igTest = { "igTest", &igTestEnter, &igTestExit, &igTestCheck};
//
// local state of buttons.  Used to optimize display
static unsigned char ls1;
static unsigned char ls2;
static unsigned char ols1;
static unsigned char ols2;
static unsigned char was_safe;

static bool safe_ok()
{
	return i_safe_ig->current_val == 0 && i_safe_main->current_val == 1;
}

/*
 * Display the button states.
 */
static void igTestDisplay()
{
	uint16_t c;

	tft.setTextColor(TM_TXT_FG_COLOR);
	tft.setTextSize(3);

	/*
	 * Need igniter not safed, mains safed, or error
	 */
	if (!safe_ok()) {
		if (was_safe == 0) {
			tft.fillRect(0,96,160,32, ST7735_RED);
			tft.setCursor(12, 100);
			tft.print("SAFE ERR");
			was_safe = 1;
		}
		return;
	}

	if (was_safe) {
		tft.fillRect(64, 96, 32, 32, TM_TXT_BKG_COLOR);
		was_safe = 0;
		ols1 = ols2 = 2;
	}

	/*
	 * Else display the input state
	 */
	if (ls1 != ols1) {
		// Erase the display area to correct color.
		// parameters are x,y of upper left, follwed by width and height
		// then color
		c = ls1? ST7735_RED: TM_TXT_BKG_COLOR;
		tft.fillRect(0, 96, 64, 32, c);	// erase the display spot
		tft.setCursor(4, 100);
		tft.print("GO");
		ols1 = ls1;
	}

	if (ls2 != ols2) {
		// Erase the display area to correct color.
		// parameters are x,y of upper left, follwed by width and height
		// then color
		c = ls2? ST7735_RED: TM_TXT_BKG_COLOR;
		tft.fillRect(96, 96, 64, 32, c);	// erase the display spot
		tft.setCursor(100, 100);
		tft.print("STOP");
		ols2 = ls2;
	}
}

/*
 * Ignition test
 * On entry, clear screen and write message
 */
void igLRTestEnter()
{
	tft.fillScreen(TM_TXT_BKG_COLOR);
	tft.setTextSize(TM_TXT_SIZE+1);
	tft.setCursor(8, TM_TXT_OFFSET);
	tft.setTextColor(TM_TXT_FG_COLOR);
	tft.print("IGNITION");
	tft.setTextSize(TM_TXT_SIZE);
	tft.setCursor(20, TM_TXT_HEIGHT+16+TM_TXT_OFFSET);
	tft.setTextColor(TM_TXT_HIGH_COLOR);
	tft.print("Test");
	// force the display routine to refresh to state "off"
	ols1 = 1;
	ls1 = 0;
	ols2 = 1;
	ls2 = 0;
	was_safe = 0;
	igTestDisplay();
}

/*
 * The state machine calls one of these once per loop().
 * If the joystick has been pressed, then leave the test.
 * Otherwise capture inputs and wait for test to start
 */
const struct state * igLocalTestEntryCheck()
{
	unsigned char e;
	unsigned int p;

	p = i_ig_pressure->filter_a;
	if (!SENSOR_SANE(p))
		return error_state(errorPressureInsane);

	if (p < min_pressure || p > max_idle_pressure)
		return error_state(errorIgNoPressure);

	if (joystick_edge_value == JOY_PRESS)
		return tft_menu_machine(&main_menu);

	if (safe_ok()) {
		ls1 = i_push_1->current_val;
		ls2 = i_push_2->current_val;
	} else
		ls1 = 0;

	igTestDisplay();

	if (ls1)
		return &igTest;

	return &igLocalTestEntry;
}

const struct state * igRemoteTestEntryCheck()
{
	unsigned char e;
	unsigned int p;

	p = i_ig_pressure->filter_a;
	if (!SENSOR_SANE(p))
		return error_state(errorPressureInsane);


	if (p < min_pressure || p > max_idle_pressure)
		return error_state(errorIgNoPressure);

	if (joystick_edge_value == JOY_PRESS)
		return tft_menu_machine(&main_menu);

	if (safe_ok()) {
		ls1 = i_cmd_1->current_val;
		ls2 = i_cmd_2->current_val;
	} else
		ls1 = 0;

	igTestDisplay();

	if (ls1)
		return &igTest;

	return &igRemoteTestEntry;
}

/*
 * Run the test here.
 */
static unsigned long run_start_t;
static unsigned long at_pressure_t;

void igTestEnter()
{
	run_start_t = loop_start_t;
	at_pressure_t = 0;
	igTestExit();
}

/*
 * On exit make sure both valves are closed (off)
 * Kill the signal to the spark generator
 */
void igTestExit()
{
	o_spark->cur_state = off;
	o_ipaIgValve->cur_state = off;
	o_n2oIgValve->cur_state = off;
	o_amberStatus->cur_state = on;
	o_greenStatus->cur_state = off;
}

/*
 * This routine is the guts of the ignition test sequence.
 * This routine is called from loop().
 */

const struct state * igTestCheck()
{
	unsigned long t;
	unsigned int p;

	p = i_ig_pressure->filter_a;

	if (!SENSOR_SANE(p))
		return error_state(errorPressureInsane);

	t = loop_start_t - run_start_t;

	// check for abort conditions

	// operator abort by remote, button 2, or joystick
	if (joystick_edge_value == JOY_PRESS ||
			i_cmd_2->current_val ||
			i_push_2->current_val)
		return error_state(errorIgTestAborted);

	// operator abort by safe switch
	if (!safe_ok())
		return error_state(errorIgTestSafe);

	// abort if pressure sensor broken
	if (p < min_pressure)
		return error_state(errorIgNoPressure);

	// abort if no ignition within time limit
	if (t > ig_pressure_time && i_ig_pressure->filter_a < good_pressure)
		return error_state(errorIgNoIg);

	// abort if chamber pressure too high
	if (p > max_ig_pressure)
		return error_state(errorIgOverPressure);

	// Run spark
	o_spark->cur_state = ((t % spark_period) < spark_period/2)? on: off;

	// Turn on valves at time
	if (t >= ig_ipa_time)
		o_ipaIgValve->cur_state = on;

	if (t >= ig_n2o_time)
		o_n2oIgValve->cur_state = on;
	
	// Status lights
	if (p > good_pressure) {
		if (at_pressure_t == 0)
			at_pressure_t = loop_start_t;
		o_amberStatus->cur_state = off;
		o_greenStatus->cur_state = on;
	} else {
		o_amberStatus->cur_state = on;
		o_greenStatus->cur_state = off;
	}

	// Stop after prescribed run time.
	if (at_pressure_t > 0 && loop_start_t - at_pressure_t > ig_run_time)
		return tft_menu_machine(&main_menu);

	return &igTest;
}
