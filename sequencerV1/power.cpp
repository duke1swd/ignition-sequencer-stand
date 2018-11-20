/*
 *  This code implements a power supply monitor "test"
 *  Input button one (left) resets the minimum report.
 *  Input button two (right) energizes the ig valves.
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

#define	LOWER_DISP	100	// where to put display of inputs.
#define	ROW1_DISP	57	// where to put display of current voltage
#define	ROW2_DISP	78	// where to display min voltage
#define	DISP_LABEL_X	4
#define	DISP_TXT	2	// text height for power displays
#define	DISP_VAL_X	(DISP_LABEL_X + DISP_TXT*6*4 + 2)
#define	DISP_INTERVAL	200	// 5 Hz period in milliseconds

void powerTestEnter();
void powerTestExit();
const struct state *powerTestCheck();
struct state powerTest = { "powerTest", &powerTestEnter, &powerTestExit, &powerTestCheck};

// local state of buttons.  Used to optimize display
static unsigned char ls1;	// edge triggered
static unsigned char els1;
static unsigned char ols1;
static unsigned char ls2;
static unsigned char ols2;
static unsigned char was_safe;
static int minPower;		// used to capture minimum power sensor value
static int curPower;		// used to capture minimum power sensor value
static int oldMinPower;
static int oldCurPower;
static unsigned long next_display_t;

// any safe state OK
static bool safe_ok()
{
	//return i_safe_ig->current_val == 0 && i_safe_main->current_val == 1;
	return 1;
}

/*
 * Print out a voltage measurement
 */
static void display_volts(int v, int y)
{
	float fv = (float)v * power_volts_per_count;
	tft.fillRect(DISP_VAL_X, y, 159, y+15, TM_TXT_BKG_COLOR);
	tft.setCursor(DISP_VAL_X, y);
	tft.print(fv);
}

/*
 * Manage display.  See igValveTest for details.
 */
static void powerDisplay()
{
	uint16_t c;

	tft.setTextColor(TM_TXT_FG_COLOR);
	tft.setTextSize(3);

	/*
	 * We don't really have a safe condition, so this code does nothing.
	 */
	if (!safe_ok()) {
		if (was_safe == 0) {
			tft.fillRect(0,LOWER_DISP - 4,160,128- LOWER_DISP, ST7735_RED);
			tft.setCursor(12, LOWER_DISP);
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
	 * Display the power values
	 */
	if (next_display_t < state_enter_t) {
		next_display_t = state_enter_t + DISP_INTERVAL;
		if (oldCurPower != curPower) {
			oldCurPower = curPower;
			display_volts(curPower, ROW1_DISP);
		}
		if (oldMinPower != minPower) {
			oldMinPower = minPower;
			display_volts(minPower, ROW2_DISP);
		}
	}


	/*
	 * Display the input state
	 */
	if (ls1 != ols1) {
		// Erase the display area to correct color.
		// parameters are x,y of upper left, follwed by width and height
		// then color
		c = ls1? ST7735_RED: TM_TXT_BKG_COLOR;
		tft.fillRect(0, 96, 64, 32, c);	// erase the display spot
		tft.setCursor(4, LOWER_DISP);
		tft.print("Min");
		ols1 = ls1;
	}

	if (ls2 != ols2) {
		// Erase the display area to correct color.
		// parameters are x,y of upper left, follwed by width and height
		// then color
		c = ls2? ST7735_RED: TM_TXT_BKG_COLOR;
		tft.fillRect(96, 96, 64, 32, c);	// erase the display spot
		tft.setCursor(100, LOWER_DISP);
		tft.print("Ig ");
		ols2 = ls2;
	}
}

/*
 * Power supply monitor test.
 * On enter, set up screen, set up state, turn of ig valves.
 */
void powerTestEnter()
{
	tft.fillScreen(TM_TXT_BKG_COLOR);
	tft.setTextSize(TM_TXT_SIZE+1);
	tft.setCursor(32, TM_TXT_OFFSET);
	tft.setTextColor(TM_TXT_FG_COLOR);
	tft.print("POWER");
	tft.setTextSize(TM_TXT_SIZE);
	tft.setCursor(50, TM_TXT_HEIGHT+16+TM_TXT_OFFSET);
	tft.setTextColor(TM_TXT_HIGH_COLOR);
	tft.print("Monitor");
	tft.setTextSize(DISP_TXT);
	tft.setCursor(DISP_LABEL_X, ROW1_DISP);
	tft.setTextColor(TM_TXT_FG_COLOR);
	tft.print("Pwr=");
	tft.setCursor(DISP_LABEL_X, ROW2_DISP);
	tft.print("Min=");

	// force the display routine to refresh to state "off"
	ols1 = 1;
	ls1 = 0;
	els1 = 0;
	ols2 = 1;
	ls2 = 0;
	was_safe = 0;
	next_display_t = 0;
	minPower = i_power_sense->filter_a;
	curPower = i_power_sense->filter_a;
	oldMinPower = 0;
	oldCurPower = 0;
	powerDisplay();
	o_ipaIgValve->cur_state = off;
	o_n2oIgValve->cur_state = off;
}

/*
 * On exit make sure both valves are closed (off)
 */
void powerTestExit()
{
	o_ipaIgValve->cur_state = off;
	o_n2oIgValve->cur_state = off;
}

/*
 * The state machine calls this once per loop().
 * If the joystick has been pressed, then leave the test.
 * Otherwise copy the input buttons to the outputs.
 */
const struct state * powerTestCheck()
{
	if (joystick_edge_value == JOY_PRESS)
		return tft_menu_machine(&main_menu);

	// make ls1 edge triggered
	ls1 = i_push_1->current_val;
	if (ls1 == 1 && els1 == 1)
		ls1 = 0;
	else
		els1 = ls1;

	ls2 = i_push_2->current_val;

	powerDisplay();
	
	// get current and minimum power supply voltage
	curPower = i_power_sense->filter_a;
	if (curPower < minPower)
		minPower = curPower;

	// button one resets the minimum
	if (ls1 == 1)
		minPower = curPower;

	if (safe_ok()) {
		// button two activates the ig valves
		if (ls2) {
			o_ipaIgValve->cur_state = on;
			o_n2oIgValve->cur_state = on;
		} else {
			o_ipaIgValve->cur_state = off;
			o_n2oIgValve->cur_state = off;
		}
	}

	return &powerTest;
}
