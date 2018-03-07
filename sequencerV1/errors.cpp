/*
 * This code displays error messages.
 * TODO: Move the screen manipulation in the check routine.
 * Is Verboten to do it in the entry routine because that delays processing outputs, which
 * may be critical.
 */

#include "parameters.h"
#include "state_machine.h"
#include "errors.h"
#include "joystick.h"
#include "tft_menu.h"
#include "io_ref.h"
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
const char e_msg_13[] PROGMEM = "Seq Oper Abort";	// Main sequence aborted by operator
const char e_msg_14[] PROGMEM = "Seq No Main Pre";	// Main sequence aborted: main did not light

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
	e_msg_13,
	e_msg_14,
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
static const struct state *l_restart_state;
static bool l_restartable;

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
 * If an error is restartable, give the state you restart to.
 */
void error_set_restart(const struct state *restart_state)
{
	l_restart_state = restart_state;
}

void error_set_restartable(bool restartable)
{
	l_restartable = restartable;
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

	// clear any edge event on remote command #1
	i_cmd_1->edge = no_edge;

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
 * Wait for joystick press.
 * For main sequence restartable errors, also look for remote control #1
 */
const struct state * erCheck()
{
	if (joystick_edge_value == JOY_PRESS) {
		if (l_restartable && l_restart_state)
			return l_restart_state;
		else
			return tft_menu_machine(&main_menu);
	}

	if (i_cmd_1->edge == rising && l_restartable) {
		i_cmd_1->edge = no_edge;
		return l_restart_state;
	}

	return &l_error_state;
}
