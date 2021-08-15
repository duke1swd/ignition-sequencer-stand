/*
 *  This code handles the remote echo test.
 *  Shows the status of the signals coming in
 *  from the opto isolators
 */

#include "state_machine.h"
#include "joystick.h"
#include "tft_menu.h"
#include "io_ref.h"
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library

extern Adafruit_ST7735 tft;
extern struct menu main_menu;

void rmEchoTestEnter();
const struct state *rmEchoTestCheck();
struct state rmEchoTest = { "rmEchoTest", &rmEchoTestEnter, NULL, &rmEchoTestCheck};

// local state of inputs.  Used to optimize display
static unsigned char ls1;
static unsigned char ls2;
static unsigned char ols1;
static unsigned char ols2;
static unsigned char was_safe;

static bool safe_ok()
{
	return i_safe_ig->current_val == 1 && i_safe_main->current_val == 1;
}

/*
 * Display the input states.  See igValveTest for comments on this routine.
 */
static void rmEchoDisplay()
{
	uint16_t c;

	tft.setTextColor(TM_TXT_FG_COLOR);
	tft.setTextSize(3);

	/*
	 * Need both igniter and mains safe for this test
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
	if (ls1 != ols1) {
		// Erase the display area to correct color.
		// parameters are x,y of upper left, follwed by width and height
		// then color
		c = ls1? ST7735_RED: TM_TXT_BKG_COLOR;
		tft.fillRect(0, 96, 64, 32, c);	// erase the display spot
		tft.setCursor(4, 100);
		tft.print("ONE");
		ols1 = ls1;
	}

	if (ls2 != ols2) {
		// Erase the display area to correct color.
		// parameters are x,y of upper left, follwed by width and height
		// then color
		c = ls2? ST7735_RED: TM_TXT_BKG_COLOR;
		tft.fillRect(96, 96, 64, 32, c);	// erase the display spot
		tft.setCursor(100, 100);
		tft.print("TWO");
		ols2 = ls2;
	}
}

/*
 * Ignition valve click-test
 * On entry, clear screen and write message
 */
void rmEchoTestEnter()
{
	tft.fillScreen(TM_TXT_BKG_COLOR);
	tft.setTextSize(TM_TXT_SIZE+1);
	tft.setCursor(8, TM_TXT_OFFSET);
	tft.setTextColor(TM_TXT_FG_COLOR);
	tft.print("Remote");
	tft.setTextSize(TM_TXT_SIZE);
	tft.setCursor(20, TM_TXT_HEIGHT+16+TM_TXT_OFFSET);
	tft.setTextColor(TM_TXT_HIGH_COLOR);
	tft.print("Echo Test");
	// force the display routine to refresh to state "off"
	ols1 = 1;
	ls1 = 0;
	ols2 = 1;
	ls2 = 0;
	rmEchoDisplay();
}

/*
 * The state machine calls this once per loop().
 * If the joystick has been pressed, then leave the test.
 * Otherwise just record the input state and display it.
 */
const struct state * rmEchoTestCheck()
{
	if (joystick_edge_value == JOY_PRESS)
		return tft_menu_machine(&main_menu);

	ls1 = i_cmd_1->current_val;
	ls2 = i_cmd_2->current_val;
	rmEchoDisplay();

	return &rmEchoTest;
}
