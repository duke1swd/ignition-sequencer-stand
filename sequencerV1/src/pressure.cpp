/*
 * This code displays the current pressure sensor readings.
 * Displays both the raw value and the calibrated PSI value.
 *
 * For sensor initialization code see sensorzero.cpp
 */

#include "state_machine.h"
#include "joystick.h"
#include "tft_menu.h"
#include "io_ref.h"
#include "pressure.h"
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library

extern Adafruit_ST7735 tft;
extern struct menu main_menu;

void pressureSensorTestEnter();
const struct state *pressureSensorTestCheck();
struct state pressureSensorTest = { "pressureSensorTest", &pressureSensorTestEnter, NULL, &pressureSensorTestCheck};

// local state of inputs.  Used to optimize display
static unsigned char was_safe;
static int16_t ig_old_p;
static int16_t main_old_p;
static unsigned long last_display_time;

static bool safe_ok()
{
	return i_safe_ig->current_val == 1 && i_safe_main->current_val == 1;
}

/*
 * Display the input states.  See igValveTest for comments on this routine.
 */
static void pressureSensorDisplay()
{
	int16_t p;
	int y;
	char refresh_ok;

	refresh_ok = 0;
	tft.setTextColor(ST7735_WHITE);
	tft.setTextSize(3);

	/*
	 * Need both igniter and mains safe for this test
	 */
	if (!safe_ok()) {
		if (was_safe == 0) {
			tft.fillRect(0,96,160,32, ST7735_RED);
			tft.setCursor(12, 100);
			tft.print(F("SAFE ERR"));
			was_safe = 1;
		}
		return;
	}

	if (was_safe) {
		tft.fillRect(0, 96, 160, 32, TM_TXT_BKG_COLOR);
		was_safe = 0;
		main_old_p = -1;	// force redisplay
	}

	tft.setTextSize(TM_TXT_SIZE);

	// Display IG label
	if (ig_old_p < 0) {
		y = 3 * TM_TXT_HEIGHT + 16 + TM_TXT_OFFSET;
		tft.setCursor(0, y);
		tft.print(F("IG"));
		ig_old_p = 0;
	}

	// Display MAIN label
	if (main_old_p < 0) {
		y = 4 * TM_TXT_HEIGHT + 16 + TM_TXT_OFFSET;
		tft.setCursor(0, y);
		tft.print(F("MN"));
		main_old_p = 0;
	}

	if (loop_start_t - last_display_time > 500)
		refresh_ok = 1;

	// Refresh displays.
	p = i_ig_pressure->filter_a;
	if (p != ig_old_p && refresh_ok) {
		ig_old_p = p;
		last_display_time = loop_start_t;
	
		// Display raw count
		y = 3 * TM_TXT_HEIGHT+16+TM_TXT_OFFSET;
		tft.fillRect(32, y, 160, y+ 2*TM_TXT_HEIGHT, TM_TXT_BKG_COLOR);
		tft.setCursor(32, y);
		tft.print(p);

		// Display PSI
		if (ig_valid && IG_PRESSURE_VALID(p)) {
			tft.setCursor(96, y);
			if (p < zero_ig)
				p = 0;
			else
				p -= zero_ig;
			p = (p * PC_SCALE) / P_SLOPE_IG;
			tft.print(p);
		}
	}

	p = i_main_press->filter_a;
	if (p != main_old_p && refresh_ok) {
		main_old_p = p;
		last_display_time = loop_start_t;
	
		y = 3 * TM_TXT_HEIGHT+16+TM_TXT_OFFSET;
		last_display_time = loop_start_t;
		y = 4 * TM_TXT_HEIGHT+16+TM_TXT_OFFSET;
		tft.fillRect(32, y, 160, y+ 2*TM_TXT_HEIGHT, TM_TXT_BKG_COLOR);
		tft.setCursor(32, y);
		tft.print(p);

		// Display PSI
		if (main_valid && MAIN_PRESSURE_VALID(p)) {
			tft.setCursor(96, y);
			if (p < zero_main)
				p = 0;
			else
				p -= zero_main;
			p = (p * PC_SCALE) / P_SLOPE_MAIN;
			tft.print(p);
		}
	}
}

/*
 * Local to opto test
 * On entry, clear screen and write message
 */
void pressureSensorTestEnter()
{
	tft.fillScreen(TM_TXT_BKG_COLOR);
	tft.setTextSize(TM_TXT_SIZE+1);
	tft.setCursor(8, TM_TXT_OFFSET);
	tft.setTextColor(TM_TXT_FG_COLOR);
	tft.print(F("Pressure"));
	tft.setTextSize(TM_TXT_SIZE);
	tft.setCursor(20, TM_TXT_HEIGHT+16+TM_TXT_OFFSET);
	tft.setTextColor(TM_TXT_HIGH_COLOR);
	tft.print(F("Sensor Test"));

	// force the display routine to refresh
	ig_old_p = -1;
	main_old_p = -1;
	last_display_time = loop_start_t;
	pressureSensorDisplay();
}

/*
 * The state machine calls this once per loop().
 * If the joystick has been pressed, then leave the test.
 * Otherwise just record the input state and display it.
 */
const struct state * pressureSensorTestCheck()
{
	if (joystick_edge_value == JOY_PRESS)
		return tft_menu_machine(&main_menu);

	pressureSensorDisplay();

	return &pressureSensorTest;
}
