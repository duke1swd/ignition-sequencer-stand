/*
 * This code dumps the data trace to the serial console
 */

#include <Arduino.h>
#include "state_machine.h"
#include "joystick.h"
#include "tft_menu.h"
#include "io_ref.h"
#include "trace.h"
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library

#ifdef TRACE

extern Adafruit_ST7735 tft;
extern struct menu main_menu;

static void traceDumpEnter();
static const struct state *traceDumpCheck();
struct state traceToSerial = { "traceToSerial", &traceDumpEnter, NULL, &traceDumpCheck};

// local state of inputs.  Used to optimize display
static unsigned char was_safe;
static bool running;
static bool was_running;
static int trace_line;

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
static void traceDumpDisplay()
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
			tft.print(F("SAFE ERR"));
			was_safe = 1;
		}
		return;
	}

	if (was_safe) {
		tft.fillRect(0, 96, 160, 32, TM_TXT_BKG_COLOR);
		was_safe = 0;
	} else if (running == was_running)
		return;

	was_running = running;

	y = 3 * TM_TXT_HEIGHT+16+TM_TXT_OFFSET;
	tft.fillRect(20, y, 160, y+ 2*TM_TXT_HEIGHT, TM_TXT_BKG_COLOR);
	tft.setTextSize(TM_TXT_SIZE);
	tft.setCursor(20, y);
	tft.print(running? "running": "done   ");
}

/*
 * Trace to Serial
 * On entry, clear screen and write message
 */
void traceDumpEnter()
{
	tft.fillScreen(TM_TXT_BKG_COLOR);
	tft.setTextSize(TM_TXT_SIZE+1);
	tft.setCursor(8, TM_TXT_OFFSET);
	tft.setTextColor(TM_TXT_FG_COLOR);
	tft.print(F("Trace"));
	tft.setTextSize(TM_TXT_SIZE);
	tft.setCursor(20, TM_TXT_HEIGHT+16+TM_TXT_OFFSET);
	tft.setTextColor(TM_TXT_HIGH_COLOR);
	tft.print(F("Trace to Serial"));

	running = true;

	// force the display routine to refresh
	was_running = true;

	trace_line = 0;
	i_push_1->edge = no_edge;
	was_safe = 0;
}

/*
 * The state machine calls this once per loop().
 * If the joystick has been pressed, then leave.
 * Otherwise update screen with our status and dump a line to serial port.
 * Dump to serial is started and stopped by pressing button #1
 */
const struct state * traceDumpCheck()
{
	if (joystick_edge_value == JOY_PRESS)
		return tft_menu_machine(&main_menu);

	traceDumpDisplay();

	if (!safe_ok())
		return current_state;

	if (i_push_1->edge == rising) {
		i_push_1->edge = no_edge;
		running = !running;
	}

	if (running) {
		if (trace_to_serial(trace_line)) {
			running = false;
			Serial.println("Done printing trace");
		} else
			trace_line += 1;
	}

	return current_state;
}
#endif
