/*
 * This code displays error messages.
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
#include <avr/pgmspace.h>    // used to hold text strings in program space.

/*
 * These are the text error messages.
 * Error numbers are defined in errors.h
 * Bad things happen if these messages are longer than 15 characters
 */
//                             |xxx xxx xxx xxx|
const char e_msg_0[]  PROGMEM = "Oper Abrt";		// test aborted by operator
const char e_msg_1[]  PROGMEM = "SAFE Abrt";		// test aborted by safe switch
const char e_msg_2[]  PROGMEM = "Ig Sensor Range";	// idle pressure out of range
const char e_msg_3[]  PROGMEM = "No Ignition";		// igniter did not come up to pressure
const char e_msg_4[]  PROGMEM = "Ig P>120";		// Over pressure in igniter
const char e_msg_5[]  PROGMEM = "Ig Sensor Fault";	// pressure sensor fault
const char e_msg_6[]  PROGMEM = "No Power";		// External power supply turned off
const char e_msg_7[]  PROGMEM = "Flame Out";		// Igniter died prematurely
const char e_msg_8[]  PROGMEM = "Mn Sensor Range";	// Main sensor out of range
const char e_msg_9[]  PROGMEM = "Mn Sesnor Fault";	// Main sensor missing or broken
const char e_msg_10[] PROGMEM = "Seq Op Abort";		// Operator abort of main sequence
const char e_msg_11[] PROGMEM = "Seq Abort Safe";	// Main sequence abort on safe switch
const char e_msg_12[] PROGMEM = "Seq Abort Power";	// Main seqeucne abort on power switch

const char * const error_messages[] PROGMEM = {
	e_msg_0,
	e_msg_1,
	e_msg_2,
	e_msg_3,
	e_msg_4,
	e_msg_5,
	e_msg_6,
	e_msg_7,
	e_msg_8,
	e_msg_9,
	e_msg_10,
	e_msg_11,
	e_msg_12,
};

char e_msg[16];

extern Adafruit_ST7735 tft;
extern struct menu main_menu;

static unsigned char error_code;	// local copy of error code
void erEnter();
void erExit();
const struct state *erCheck();
struct state l_error_state = { "error display", &erEnter, &erExit, &erCheck};

static int next_event_to_daq;

/*
 * Called by a check routine.  Saves the
 * error number and returns the error state
 * structure
 */
const struct state *error_state(unsigned char code)
{
	error_code = code;
	return &l_error_state;
}

/*
 * When we enter the routine, display the error code
 */
void
erEnter()
{
	next_event_to_daq = 0;

	o_redStatus->cur_state = on;
	o_amberStatus->cur_state = off;
	o_greenStatus->cur_state = off;

	o_daq0->cur_state = off;
	o_daq1->cur_state = off;

	// background is RED
	tft.fillScreen(ST7735_RED);
	tft.setTextSize(TM_TXT_SIZE+1);
	tft.setCursor(8, TM_TXT_OFFSET);
	// text is white
	tft.setTextColor(ST7735_WHITE);
	tft.print("ERROR");
	tft.setTextSize(TM_TXT_SIZE);
	tft.setCursor(20, TM_TXT_HEIGHT+16+TM_TXT_OFFSET);
	tft.print(error_code);

	// load up and display the error message
	if (error_code >= 1 && error_code <= NUM_ERRORS) {
		strcpy_P(e_msg, (char*)pgm_read_word(&(error_messages[error_code-1])));
		tft.setCursor(0, 2 * TM_TXT_HEIGHT+16+TM_TXT_OFFSET);
		tft.print(e_msg);
	}
}

void
erExit()
{
	// normal run states have amber on, other LEDs off.
	o_redStatus->cur_state = off;
	o_amberStatus->cur_state = on;
	o_greenStatus->cur_state = off;
	o_daq1->cur_state = off;
}

/*
 * Dump the event log to daq
 * Wait for joystick press.
 */
const struct state * erCheck()
{
	if (joystick_edge_value == JOY_PRESS)
		return tft_menu_machine(&main_menu);
	return &l_error_state;
}
