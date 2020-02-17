/*
 *  This code handles the ignition valve click test
 */

#include "state_machine.h"
#include "joystick.h"
#include "tft_menu.h"
#include "io_ref.h"
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library

extern Adafruit_ST7735 tft;
extern struct menu main_menu;

void igValveTestEnter();
void igValveTestExit();
const struct state *igValveTestCheck();
struct state igValveTest = { "igValveTest", &igValveTestEnter, &igValveTestExit, &igValveTestCheck};

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
 * Lots of display constants here.
 * If you use a different display you'll need to make edits here.
 * Basic layout: screen is 128 high by 160 wide
 * Each button is displayed in one of the two bottom corners.
 * Display area of a button is 64 wide by 32 high
 *
 * Button display background is red if button pressed, else black
 * Text in display is always white.
 */
static void igButtonDisplay()
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
		tft.print("IPA");
		ols1 = ls1;
	}

	if (ls2 != ols2) {
		// Erase the display area to correct color.
		// parameters are x,y of upper left, follwed by width and height
		// then color
		c = ls2? ST7735_RED: TM_TXT_BKG_COLOR;
		tft.fillRect(96, 96, 64, 32, c);	// erase the display spot
		tft.setCursor(100, 100);
		tft.print("N2O");
		ols2 = ls2;
	}
}

/*
 * Ignition valve click-test
 * On entry, clear screen and write message
 */
void igValveTestEnter()
{
	tft.fillScreen(TM_TXT_BKG_COLOR);
	tft.setTextSize(TM_TXT_SIZE+1);
	tft.setCursor(8, TM_TXT_OFFSET);
	tft.setTextColor(TM_TXT_FG_COLOR);
	tft.print("Ig Valve");
	tft.setTextSize(TM_TXT_SIZE);
	tft.setCursor(20, TM_TXT_HEIGHT+16+TM_TXT_OFFSET);
	tft.setTextColor(TM_TXT_HIGH_COLOR);
	tft.print("Click Test");
	// force the display routine to refresh to state "off"
	ols1 = 1;
	ls1 = 0;
	ols2 = 1;
	ls2 = 0;
	was_safe = 0;
	igButtonDisplay();
}

/*
 * On exit make sure both valves are closed (off)
 */
void igValveTestExit()
{
	o_ipaIgValve->cur_state = off;
	o_n2oIgValve->cur_state = off;
}

/*
 * The state machine calls this once per loop().
 * If the joystick has been pressed, then leave the test.
 * Otherwise copy the input buttons to the outputs.
 */
#define	ON_TIME	10000	// ten seconds
const struct state * igValveTestCheck()
{
	unsigned char new_s1;
	static long press_time;
	if (joystick_edge_value == JOY_PRESS)
		return tft_menu_machine(&main_menu);

	if (safe_ok()) {
#ifdef ON_TIME
		new_s1 = i_push_1->current_val;
		if (new_s1 == 1 && new_s1 != ols1)
			press_time = loop_start_t;
		if (new_s1 == 1 || press_time - loop_start_t < ON_TIME)
			ls1 = 1;
		else
			ls1 = 0;

		o_ipaIgValve->cur_state = ls1? on: off;
#else
		o_ipaIgValve->cur_state = (ls1 = i_push_1->current_val)? on: off;
#endif
		o_n2oIgValve->cur_state = (ls2 = i_push_2->current_val)? on: off;
	}
	igButtonDisplay();

	return &igValveTest;
}
