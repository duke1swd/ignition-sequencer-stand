/*
 * This code displays error messages.
 */

#include "parameters.h"
#include "state_machine.h"
#include "errors.h"
#include "joystick.h"
#include "tft_menu.h"
#include "io_ref.h"
#include "events.h"
#include "trace.h"
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
const char e_msg_9[]  PROGMEM = "Mn Sensor Fault";	// Main sensor missing or broken
const char e_msg_10[] PROGMEM = "Seq Op Abort";		// Operator abort of main sequence
const char e_msg_11[] PROGMEM = "Seq Abort Safe";	// Main sequence abort on safe switch
const char e_msg_12[] PROGMEM = "Seq Abort Power";	// Main seqeucne abort on power switch
const char e_msg_13[] PROGMEM = "Seq Oper Abort";	// Main sequence aborted by operator
const char e_msg_14[] PROGMEM = "Seq No Main Pre";	// Main sequence aborted: main did not light
const char e_msg_15[] PROGMEM = "Ig Press < Main";	// Igniter pressure too low when main is up to pressure

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
	e_msg_15,
};

char e_msg[16];

extern Adafruit_ST7735 tft;
extern struct menu main_menu;

static unsigned char error_code;	// local copy of error code
static unsigned int error_value;	// local copy of the error value
static bool error_value_is_present;
void erEnter();
void erExit();
const struct state *erCheck();
struct state l_error_state = { "error display", &erEnter, &erExit, &erCheck};

static const struct state *l_restart_state;
/* 
 * 0 = not restartable
 * 1 = restartable
 * 2 = no longer restartable on the same error
 */
static unsigned char l_restartable;
static unsigned char last_restartable_error;
static bool do_entry_stuff;

#define	NO_BLINK	0xff
#define	BLINK_HALF_PERIOD	500	// half a second on, half a second off.
#define	INTERBLINK	(BLINK_HALF_PERIOD * 4)
static unsigned char e_blink_state;
static unsigned long next_blink_time;


/*
 * Called by a check routine.  Saves the
 * error number and returns the error state
 * structure
 */
static const struct state *i_error_state(unsigned char code)
{
	error_code = code;

	/*
	 * Don't restart on the second occurance of the same error,
	 * but a new error code is still restartable.
	 */
	if (error_code != last_restartable_error) {
		last_restartable_error = error_code;
		if (l_restartable > 1)
			l_restartable = 1;
	}
#ifdef TRACE_PIN
	trace_trigger();
#endif
	return &l_error_state;
}

const struct state *error_state(unsigned char code)
{
	error_value_is_present = false;
	return i_error_state(code);
}

const struct state *error_state(unsigned char code, unsigned int value)
{
	error_value = value;
	error_value_is_present = true;
	return i_error_state(code);
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
	if (restartable)
		l_restartable++;
	else
		l_restartable = 0;
	if (l_restartable > 2)
		l_restartable = 2;
}

/*
 * When we enter the routine, display the error code
 */
void
erEnter()
{
	if (l_restartable == 1) {
		o_redStatus->cur_state = off;
		o_amberStatus->cur_state = on;
	} else {
		o_redStatus->cur_state = on;
		o_amberStatus->cur_state = off;
	}
	o_greenStatus->cur_state = off;

	o_daq0->cur_state = off;
	o_daq1->cur_state = off;

	// clear any edge event on remote command #1
	i_cmd_1->edge = no_edge;
	do_entry_stuff = true;

	// Initialize blink state machine
	e_blink_state = NO_BLINK;
	next_blink_time = loop_start_t + INTERBLINK;
}

/*
 * This code used to be in the state entry routine.
 * However this code takes > 100 ms to run.  Since output
 * processing happens *after* the entry routine, and
 * since output transitions on an error may be time critical,
 * we delay the entry manipulation of the screen
 * into the check routine.
 *
 * For this same reason, we delay entry_stuff until any tracing post
 * trigger is complete.
 */
static void i_do_entry_stuff()
{
	if (!do_entry_stuff)
		return;
#ifdef TRACE_PIN
	if (!trace_done())
		return;
	trace_commit();
#endif // TRACE_PIN

	do_entry_stuff = false;

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
		if (error_value_is_present) {
			tft.setCursor(0, 3 * TM_TXT_HEIGHT+16+TM_TXT_OFFSET);
			tft.print(error_value);
		}
	}

	/*
	 * Write the event log to eeprom.
	 * WARNING: this can take a while.
	 */
	event_commit_conditional();
}

void
erExit()
{
	// normal run states have amber on, other LEDs off.
	o_redStatus->cur_state = off;
	o_amberStatus->cur_state = on;
	o_greenStatus->cur_state = off;
	o_daq1->cur_state = off;
#ifdef TRACE
	trace_init();
#endif
}

/*
 * Wait for joystick press.
 * For main sequence restartable errors, also look for remote control #1
 */
const struct state * erCheck()
{
	unsigned char t, v;
	i_do_entry_stuff();

	if (joystick_edge_value == JOY_PRESS) {
		if (l_restartable == 1 && l_restart_state)
			return l_restart_state;
		else
			return tft_menu_machine(&main_menu);
	}

	if (i_cmd_1->edge == rising && l_restartable == 1 && l_restart_state) {
		i_cmd_1->edge = no_edge;
		return l_restart_state;
	}

	// blink the code on the LEDs.
	// Code is blinked in octal.  RED led blinks the number of 8s, AMBER the number of 1s
	if (loop_start_t >= next_blink_time) {
		o_redStatus->cur_state = off;
		o_amberStatus->cur_state = off;

		next_blink_time += BLINK_HALF_PERIOD;
		e_blink_state += 1;

		if ((e_blink_state & 1) == 0) {
			t = (error_code & 0x38) / 4;	// divide by 8, multiply by 2
			v = (error_code & 0x7) * 2;	// modulo 8, multiply by 2
			if (e_blink_state < t) {
				o_redStatus->cur_state = on;	// blink out the number of 8s in RED
			} else if (e_blink_state - t < v) {
				o_amberStatus->cur_state = on;	// blink out the 1s digit in AMBER
			} else {
				e_blink_state = NO_BLINK;
				next_blink_time += INTERBLINK - BLINK_HALF_PERIOD;
			}
		}
	}


	return &l_error_state;
}
