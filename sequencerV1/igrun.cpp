/*
 *  This code handles entry into the ignition system test
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

extern Adafruit_ST7735 tft;
extern struct menu main_menu;

void igLRTestEnter();
void igLRDebugEnter();
void igLongTestEnter();
void igLongTestRunEnter();
const struct state *igLocalTestEntryCheck();
const struct state *igRemoteTestEntryCheck();
const struct state *igLongTestEntryCheck();
const struct state *igLongTestRunCheck();
struct state igLocalTestEntry = { "igLocalTest", &igLRTestEnter, NULL, &igLocalTestEntryCheck};
struct state igLocalDebugEntry = { "igLocalDebug", &igLRDebugEnter, NULL, &igLocalTestEntryCheck};
struct state igRemoteTestEntry = { "igRemoteTest", &igLRTestEnter, NULL, &igRemoteTestEntryCheck};
struct state igLongTestEntry = { "igLongTestEnter", &igLongTestEnter, NULL, &igLongTestEntryCheck};
struct state igLongTestRun = { "igLongTestRun", &igLongTestRunEnter, NULL, &igLongTestRunCheck};
extern struct state runStart;
extern struct state runIgDebug;
const struct state *igThisTest;
//
// local state of buttons.  Used to optimize display
static unsigned char ls1;
static unsigned char ls2;
static unsigned char ols1;
static unsigned char ols2;
static unsigned char was_safe;
static unsigned char was_power;

unsigned char igDebug;
static char* testname;
void common_test_enter();

static bool safe_ok()
{
	return i_safe_ig->current_val == 0 && i_safe_main->current_val == 1;
}

static bool power_ok()
{
	return i_power_sense->current_val == 1;
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
	if (!power_ok()) {
		if (was_power == 0) {
			tft.fillRect(0,96,160,32, ST7735_RED);
			tft.setCursor(12, 100);
			tft.setTextColor(ST7735_WHITE);
			tft.print("NO POWER");
			was_power = 1;
		}
		return;
	}
	if (!safe_ok()) {
		if (was_safe == 0) {
			tft.fillRect(0,96,160,32, ST7735_RED);
			tft.setCursor(12, 100);
			tft.setTextColor(ST7735_WHITE);
			tft.print("SAFE ERR");
			was_safe = 1;
		}
		return;
	}

	if (was_safe || was_power) {
		tft.fillRect(64, 96, 32, 32, TM_TXT_BKG_COLOR);
		was_safe = 0;
		was_power = 0;
		ols1 = ols2 = 2;	// force redisplay by setting to nonsense value
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
 * Ignition Debug.  Entry is same as Ignition Test
 */
void igLRDebugEnter()
{
	igLRTestEnter();
}

/*
 * Ignition test
 * On entry, clear screen and write message
 */
void igLRTestEnter()
{
	testname = "IGNITION";
	common_test_enter();
	igDebug = false;
}

void common_test_enter()
{
	extern const struct state * current_state;

	igDebug = true;
	tft.fillScreen(TM_TXT_BKG_COLOR);
	tft.setTextSize(TM_TXT_SIZE+1);
	tft.setCursor(8, TM_TXT_OFFSET);
	tft.setTextColor(TM_TXT_HIGH_COLOR);
	tft.print(testname);
	tft.setTextSize(TM_TXT_SIZE);
	tft.setCursor(20, TM_TXT_HEIGHT+16+TM_TXT_OFFSET);
	tft.setTextColor(TM_TXT_FG_COLOR);
	tft.print("Test");
	// force the display routine to refresh to state "off"
	ols1 = 1;
	ls1 = 0;
	ols2 = 1;
	ls2 = 0;
	was_safe = 0;
	was_power = 0;
	igTestDisplay();
	igThisTest = current_state;
	runInit(RUN_IG_ONLY);
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

	if (safe_ok() && power_ok()) {
		ls1 = i_push_1->current_val;
		ls2 = i_push_2->current_val;
	} else
		ls1 = 0;

	igTestDisplay();

	if (ls1) {
		if (igDebug)
			return &runIgDebug;
		return &runStart;
	}

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

	if (safe_ok() && power_ok()) {
		ls1 = i_cmd_1->current_val;
		ls2 = i_cmd_2->current_val;
	} else
		ls1 = 0;

	igTestDisplay();

	if (ls1) {
		if (igDebug)
			return &runIgDebug;
		return &runStart;
	}

	return &igRemoteTestEntry;
}

/*
 * Code to handle the long test
 */

#define	N_TESTS		7	// number of iterations
#define	TIME_BETWEEN	10000	// 10 seconds in milliseconds
static char test_count;

/*
 * Long Ignition test
 * On entry, clear screen and write message
 */
void igLongTestEnter()
{
	testname = "IG LONG";
	test_count = 0;
	common_test_enter();
}

/*
 * The state machine calls one of these once per loop().
 * If the joystick has been pressed, then leave the test.
 * Otherwise capture inputs and wait for test to start
 * This test can be run by either remote or local buttons.
 */
const struct state * igLongTestEntryCheck()
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

	if (safe_ok() && power_ok()) {
		ls1 = (i_push_1->current_val || i_cmd_1->current_val);
		ls2 = (i_push_2->current_val || i_cmd_2->current_val);
	} else
		ls1 = 0;

	igTestDisplay();

	if (ls1)
		return &igLongTestRun;

	return &igLongTestEntry;
}

/*
 * This state runs the test over and over again.
 */
void igLongTestRunEnter()
{
	extern const struct state * current_state;
	igThisTest = current_state;

	test_count++;
	
	// erase the screen
	tft.fillScreen(TM_TXT_BKG_COLOR);
	if (test_count > N_TESTS)
		return;

	// Display test number
	tft.setTextSize(TM_TXT_SIZE+2);
	tft.setCursor(16, TM_TXT_OFFSET);
	tft.setTextColor(TM_TXT_HIGH_COLOR);
	tft.print((int)test_count);
	tft.setTextSize(TM_TXT_SIZE);
}

const struct state * igLongTestRunCheck()
{
	unsigned long t;

	t = loop_start_t - state_enter_t;
	if (loop_start_t < state_enter_t)
		t = 0;

	// If joystick, or button 2, return to menu system.
	if (joystick_edge_value == JOY_PRESS ||
	    i_push_2->current_val ||
	    i_cmd_2->current_val)
		return tft_menu_machine(&main_menu);

	// If we have done all the tests then go back to entry state
	if (test_count > N_TESTS)
		return &igLongTestEntry;
	
	// If the waiting period is over, then run the next test.
	if (test_count == 1 || t > TIME_BETWEEN)
		return &runIgDebug;

	return current_state;
}
