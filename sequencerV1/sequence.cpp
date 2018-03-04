/*
 * This code contains code specific to the main sequence run.
 *
 * States used in this module
 * 	sequenceStart
 * 		This state sets things up and immediately exist to runStart
 * 		(see run.cpp)
 *	sequence_light_main
 *		This state lights the main chamber
 *	sequence_run
 *		This state runs the main chamber.  Exit is by time.
 *	sequence_report
 *		This state dumps the event log to the DAQ.
 */

#include "parameters.h"
#include "state_machine.h"
#include "errors.h"
#include "joystick.h"
#include "tft_menu.h"
#include "io_ref.h"
#include "run.h"
#include "events.h"
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library

extern void mainIPAOpen();
extern void mainIPAClose();
extern void mainN2OOpen();
extern void mainN2OClose();
extern void spark_run();
extern Adafruit_ST7735 tft;
extern struct menu main_menu;
extern struct state runStart;

void sequenceEntryEnter();
void sequenceEntryExit();
const struct state *sequenceEntryCheck();
struct state sequenceEntry = { "sequenceEntry", &sequenceEntryEnter, &sequenceEntryExit, &sequenceEntryCheck};

static bool was_power;	// true if last iteration we displayed the power error message
static bool was_safe;	// true if last iteration we displayed the safe error message

static bool safe_ok()
{
	// Igniter must not be safe
	if (i_safe_ig->current_val == 1)
		return 0;

	// Main must not be safe
	if (i_safe_main->current_val == 1)
		return 0;

	return 1;
}

static bool power_ok()
{
	return i_power_sense->current_val == 1;
}

/*
 * On Sequence Entry, set up the display and the LEDs
 */
void sequenceEntryEnter()
{
	tft.fillScreen(TM_TXT_BKG_COLOR);
	tft.setTextSize(TM_TXT_SIZE+1);
	tft.setCursor(8, TM_TXT_OFFSET);
	tft.setTextColor(TM_TXT_HIGH_COLOR);
	tft.print("Main Sequence");
	tft.setTextSize(TM_TXT_SIZE);
	tft.setCursor(20, TM_TXT_HEIGHT+16+TM_TXT_OFFSET);
	tft.setTextColor(TM_TXT_FG_COLOR);
	tft.print("RUN");
	
	was_safe = 0;
	was_power = 0;
}

/*
 * On Sequence Exit, erase the screen and set the LEDs to RED.
 * Note that on entrance to a good state we'll reset the LEDs
 * to a more pleasing combination.
 */
void sequenceEntryExit() {
	tft.fillScreen(TM_TXT_BKG_COLOR);
	o_greenStatus->cur_state = off;
	o_amberStatus->cur_state = off;
	o_redStatus->cur_state = on;
	o_daq0->cur_state = off;
	o_daq1->cur_state = off;
}


/*
 * The purpose of this state is to verify we are good to go and
 * wait for the command to start.
 *
 * This routine runs once per loop.
 * If anything goes wrong, abort to an error.
 * 
 * During this state, copy cmd_1 to daq_0
 */
const struct state * sequenceEntryCheck()
{
	unsigned int p;
	extern const struct state * current_state;

	// copy the command input button to the daq.
	// This allows remote daq start.
	o_daq0->cur_state = (i_cmd_1->current_val == 1? on: off);

	// until proven otherwise, LEDs indicate not-ready
	o_greenStatus->cur_state = off;
	o_amberStatus->cur_state = off;
	o_redStatus->cur_state = on;

	p = i_ig_pressure->filter_a;
	if (!SENSOR_SANE(p))
		return error_state(errorIgPressureInsane);

	if (p < min_pressure || p > max_idle_pressure)
		return error_state(errorIgNoPressure);

	p = i_main_press->filter_a;
	if (!SENSOR_SANE(p))
		return error_state(errorMainPressureInsane);

	if (p < min_pressure || p > max_idle_pressure)
		return error_state(errorMainNoPressure);

	if (joystick_edge_value == JOY_PRESS)
		return tft_menu_machine(&main_menu);

	/*
	 * Display the status
	 */
	tft.setTextColor(TM_TXT_FG_COLOR);
	tft.setTextSize(3);

	/*
	 * Verify safe and power switches OK
	 */
	if (!power_ok()) {
		if (!was_power) {
			tft.fillRect(0,96,160,32, ST7735_RED);
			tft.setCursor(12, 100);
			tft.setTextColor(ST7735_WHITE);
			tft.print("NO POWER");
			was_power = true;
		}
		return (current_state);
	}

	if (!safe_ok()) {
		if (!was_safe) {
			tft.fillRect(0,96,160,32, ST7735_RED);
			tft.setCursor(12, 100);
			tft.setTextColor(ST7735_WHITE);
			tft.print("SAFE ERR");
			was_safe = true;
		}
		return (current_state);
	}


	/*
	 * Else we are ready
	 */
	o_greenStatus->cur_state = on;
	o_amberStatus->cur_state = off;
	o_redStatus->cur_state = off;

	if (was_safe || was_power) {
		tft.fillRect(0,96,160,32, ST7735_GREEN);
		was_safe = false;
		was_power = false;
	}

	if (i_cmd_2->current_val) {
		event_enable();
		i_cmd_2->edge = no_edge; // runStart will abort of rising edge of i_cmd_2
		return &runStart;
	}

	return (current_state);
}
