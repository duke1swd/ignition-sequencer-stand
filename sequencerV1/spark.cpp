/*
 *  This code handles the spark test
 */

#include "parameters.h"
#include "state_machine.h"
#include "joystick.h"
#include "tft_menu.h"
#include "io_ref.h"
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library

extern Adafruit_ST7735 tft;
extern struct menu main_menu;

void sparkTestEnter();
void sparkTestExit();
const struct state *sparkTestCheck();
struct state sparkTest = { "sparkTest", &sparkTestEnter, &sparkTestExit, &sparkTestCheck};

// local state of buttons.  Used to optimize display
static unsigned char ls1;	// edge triggered
static unsigned char els1;
static unsigned char ols1;
static unsigned char ls2;
static unsigned char ols2;
static unsigned char was_safe;

static bool safe_ok()
{
	return i_safe_ig->current_val == 0 && i_safe_main->current_val == 1;
}

/*
 * Manage display.  See igValveTest for details.
 */
static void sparkButtonDisplay()
{
	uint16_t c;

	tft.setTextColor(TM_TXT_FG_COLOR);
	tft.setTextSize(3);

	/*
	 * Need igniter and mains safed, or error
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
		tft.print("one");
		ols1 = ls1;
	}

	if (ls2 != ols2) {
		// Erase the display area to correct color.
		// parameters are x,y of upper left, follwed by width and height
		// then color
		c = ls2? ST7735_RED: TM_TXT_BKG_COLOR;
		tft.fillRect(96, 96, 64, 32, c);	// erase the display spot
		tft.setCursor(100, 100);
		tft.print("RUN");
		ols2 = ls2;
	}
}

/*
 * Ignition valve click-test
 * On entry, clear screen and write message
 */
void sparkTestEnter()
{
	tft.fillScreen(TM_TXT_BKG_COLOR);
	tft.setTextSize(TM_TXT_SIZE+1);
	tft.setCursor(32, TM_TXT_OFFSET);
	tft.setTextColor(TM_TXT_FG_COLOR);
	tft.print("SPARK");
	tft.setTextSize(TM_TXT_SIZE);
	tft.setCursor(50, TM_TXT_HEIGHT+16+TM_TXT_OFFSET);
	tft.setTextColor(TM_TXT_HIGH_COLOR);
	tft.print("Test");
	// force the display routine to refresh to state "off"
	ols1 = 1;
	ls1 = 0;
	els1 = 0;
	ols2 = 1;
	ls2 = 0;
	was_safe = 0;
	sparkButtonDisplay();
	o_ipaIgValve->cur_state = off;
	o_n2oIgValve->cur_state = off;
}

/*
 * On exit make sure both valves are closed (off)
 */
void sparkTestExit()
{
	o_spark->cur_state = off;
}

/*
 * The state machine calls this once per loop().
 * If the joystick has been pressed, then leave the test.
 * Otherwise copy the input buttons to the outputs.
 */
const struct state * sparkTestCheck()
{
	static unsigned long run_start_t;
	unsigned long t;

	if (joystick_edge_value == JOY_PRESS)
		return tft_menu_machine(&main_menu);

	// make ls1 edge triggered
	ls1 = i_push_1->current_val;
	if (ls1 == 1 && els1 == 1)
		ls1 = 0;
	else
		els1 = ls1;

	ls2 = i_push_2->current_val;
	// Cannot do both tests.
	if (ls1 || els1)
		ls2 = 0;

	if (ls2 == 1 && ols2 == 0)
		run_start_t = loop_start_t;

	sparkButtonDisplay();

	if (safe_ok()) {
		// one-shot on button one.
		o_spark->cur_state = ls1? on: off;

		// continuous run on button two
		if (ls2) {
			t = (loop_start_t - run_start_t) % spark_period;
			o_spark->cur_state = (t == 0)? on: off;
		}
	}

	return &sparkTest;
}
