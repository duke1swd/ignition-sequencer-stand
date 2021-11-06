/*
 * This code displays the current pressure sensor reading
 */

#include "state_machine.h"
#include "joystick.h"
#include "tft_menu.h"
#include "io_ref.h"
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library

extern Adafruit_ST7735 tft;
extern struct menu main_menu;

void pressureSensorTestEnter();
const struct state *pressureSensorTestCheck();
struct state pressureSensorTest = { "pressureSensorTest", &pressureSensorTestEnter, NULL, &pressureSensorTestCheck};

// local state of inputs.  Used to optimize display
static unsigned char was_safe;
static int32_t old_p;

static bool safe_ok()
{
	return i_safe_ig->current_val == 1 && i_safe_main->current_val == 1;
}

/*
 * Display the input states.  See igValveTest for comments on this routine.
 */
static void pressureSensorDisplay()
{
	int32_t p;
	int y;

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
	}

	// p is the filtered pressure (counts * 4)
	p = i_ig_pressure->filter_a;
	if (p == old_p)
		return;
	old_p = p;
	y = 3 * TM_TXT_HEIGHT+16+TM_TXT_OFFSET;
	tft.fillRect(20, y, 160, y+ 2*TM_TXT_HEIGHT, TM_TXT_BKG_COLOR);
	tft.setTextSize(TM_TXT_SIZE);
	tft.setCursor(20, y);
	tft.print(p);

	// Convert P into PSI.
	//      0 = 0.0 volts
	//  409.6 = 0.5 volts, 0 PSI
	// 3276.8 / 500 counts per PSI
	// 4096 = 5.0 volts
	// 
	p *= 10;
	p -= 4096;	// 0 PSI = 0.5 volts
	p = p * 500 / 32768;
	y += TM_TXT_HEIGHT;
	tft.setCursor(20, y);
	tft.print(p);
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
	// force the display routine to refresh to state "off"
	old_p = -1;
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
