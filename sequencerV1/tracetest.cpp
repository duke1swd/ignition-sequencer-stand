/*
 * Test the signal tracing code
 */

#include <Arduino.h>
#include "state_machine.h"
#include "joystick.h"
#include "tft_menu.h"
#include "io_ref.h"
#include "trace.h"
#include "errors.h"
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library

#ifdef TRACE

extern Adafruit_ST7735 tft;
extern struct menu main_menu;

static void traceTestEnter();
static const struct state *traceTestCheck();
struct state traceTest = { "traceTest", &traceTestEnter, NULL, &traceTestCheck};

// local state of inputs.  Used to optimize display
static unsigned char was_safe;

/*
 * Want both switches to safe.
 */
static bool safe_ok()
{
	return i_safe_ig->current_val == 1 && i_safe_main->current_val == 1;
}

/*
 * Display the input states.  See igValveTest for comments on this routine.
 */
static void traceTestDisplay()
{
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
			tft.print("SAFE ERR");
			was_safe = 1;
		}
		return;
	}

	if (was_safe) {
		tft.fillRect(0, 96, 160, 32, TM_TXT_BKG_COLOR);
		tft.setCursor(4, 100);
		tft.print("Stop");
		was_safe = 0;
	}
}

/*
 * Trace test
 * On entry, clear screen and write message
 */
void traceTestEnter()
{
	tft.fillScreen(TM_TXT_BKG_COLOR);
	tft.setTextSize(TM_TXT_SIZE+1);
	tft.setCursor(8, TM_TXT_OFFSET);
	tft.setTextColor(TM_TXT_FG_COLOR);
	tft.print("Trace");
	tft.setTextSize(TM_TXT_SIZE);
	tft.setCursor(20, TM_TXT_HEIGHT+16+TM_TXT_OFFSET);
	tft.setTextColor(TM_TXT_HIGH_COLOR);
	tft.print("Trace Test");
	tft.setCursor(20, 3 * TM_TXT_HEIGHT+16+TM_TXT_OFFSET);
	tft.print("running");

	i_push_1->edge = no_edge;
	was_safe = 0;
}

/*
 * The state machine calls this once per loop().
 * If the joystick has been pressed, then leave.
 * Otherwise wait for button 1.  When pressed,
 * exit with an error.
 */
const struct state * traceTestCheck()
{
	if (joystick_edge_value == JOY_PRESS)
		return tft_menu_machine(&main_menu);

	traceTestDisplay();

	if (!safe_ok())
		return current_state;

	if (i_push_1->edge == rising) {
		i_push_1->edge = no_edge;
		return error_state(errorIgTestAborted);
	}

	return current_state;
}
#endif
