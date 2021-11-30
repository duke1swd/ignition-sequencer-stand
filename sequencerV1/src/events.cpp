/*
 * Code to record events.
 * Events are only used by the main sequence.
 *
 * Events are logged to an in memory buffer.
 * The committed event log is written to EEPROM
 */

#include "Arduino.h"
#include "EEPROM.h"
#include "avr/pgmspace.h"
#include "parameters.h"
#include "state_machine.h"
#include "events.h"
#include "io_ref.h"
#include "sendtodaq.h"
#include "eepromlocal.h"
#include "event_names.h"

// Events are 6 bytes long.  Storing 250 of them takes 1500 bytes.
// This is about a quarter of the available ram (6KB for static, 2KB for stack)
static struct event_s {
	enum event_codes e_e;
	unsigned int time_e;
	unsigned int param_e;
} event_buffer[EVENT_BUFFER_SIZE];

static struct event_s *e_p;

static unsigned long event_base_t;

static int n_events;
static bool enabled;


/*
 * Initialize the event system, discarding any events that may be
 */
void event_init()
{
	enabled = false;
	event_base_t = 0;	// don't have a base time yet.
	n_events = 0;
	e_p = event_buffer;
}

void event_enable()
{
	enabled = true;
	n_events = 0;
	e_p = event_buffer;
}

void event_disable()
{
	enabled = false;
}

/*
 * Called to record an event.
 * Pass in the event.  loop_start_t is used for the timestamp
 */
void event(enum event_codes e, unsigned int p) {

	if (!enabled)
		return;

	if (event_base_t == 0)
		event_base_t = loop_start_t;

	// Record the event if we have space
	if (n_events < EVENT_BUFFER_SIZE) {
		e_p->e_e = e;
		e_p->time_e = loop_start_t - event_base_t;
		e_p->param_e = p;
		e_p++;
	}

	n_events++;
}

/*
 * Write the event log to EEPROM
 *
 * Returns the log sequence number.
 */
unsigned int event_commit() {
	int i, n;
	unsigned int seqn;

	if (n_events <= 0)
		return 0xffff;

	// mark the in-eeprom event log as invalid
	i = 0;
	EEPROM.put(EEPROM_EVENT_SIZE_2, i);

	// record the size of the event log
	n = min(n_events, EVENT_BUFFER_SIZE);
	EEPROM.put(EEPROM_EVENT_SIZE, n);
	Serial.print("Writing "); Serial.print(n); Serial.print(" events to EEPROM\n");
	
	// write the events
	for (i = 0; i < n; i++)
		EEPROM.put(EEPROM_EVENT_LOG + i * sizeof (struct event_s),
				event_buffer[i]);

	// update the log seqn number in eeprom
	EEPROM.get(EEPROM_EVENT_SEQN, seqn);
	seqn += 1;
	EEPROM.put(EEPROM_EVENT_SEQN, seqn);

	EEPROM.put(EEPROM_EVENT_SIZE_2, n);

	return seqn;
}

/*
 * Called by error states to commit.
 * Does the required checks.
 * Also, sends the seqn to the DAQ.
 */
void event_commit_conditional() {
	unsigned int seqn;

	if (!enabled || n_events == 0)
		return;

	enabled = false;
	seqn = event_commit();
	n_events = 0;

	send_som();
	send_byte((unsigned char)MY_EEPROM_MAGIC_NUMBER);
	send_long((unsigned long)seqn);
	send_eom();
}


/*
 * Write the log to Serial.
 * In order to allow this to be interrupted, we write 1 line at a time.
 * Returns false normally.
 * Returns true when there are no more lines to print.
 * Caller is responsible for starting _i_ at zero and incrementing it.
 * If it returns true on i=0, then no log exists.
 */
static int n_eeprom_events;
static char buffer[EVENT_MAX_CODE_LENGTH];

bool event_to_serial(int i) {
	int n;
	unsigned int seqn;
	struct event_s l_event;
	unsigned int l_t;


	// If i=0, print the header and check valid
	if (i == 0) {
		EEPROM.get(EEPROM_EVENT_SIZE, n_eeprom_events);
		EEPROM.get(EEPROM_EVENT_SIZE_2, n);
		if (n != n_eeprom_events) {
			n_eeprom_events = 0;
			return true;
		}
		
		EEPROM.get(EEPROM_EVENT_SEQN, seqn);
		Serial.print("Log #: ");
		Serial.print(seqn);
		Serial.print("  has ");
		Serial.print(n_eeprom_events);
		Serial.print(" events\n");
		return false;
	}
	i -= 1;

	// else print a log event
	if (i < 0 || i >= n_eeprom_events || i >= EVENT_BUFFER_SIZE)
		return true;

	EEPROM.get(EEPROM_EVENT_LOG + i * sizeof (struct event_s), l_event);
	l_t = l_event.time_e;
	if (l_t < 10000)
		Serial.print(" ");
	if (l_t < 1000)
		Serial.print(" ");
	if (l_t < 100)
		Serial.print(" ");
	if (l_t < 10)
		Serial.print(" ");
	Serial.print(l_t);
	Serial.print(": ");

	// Necessary casts and dereferencing, just copy.
	strcpy_P(buffer, (char*)pgm_read_word(&(event_code_names[l_event.e_e])));
	Serial.print(buffer);
	Serial.print("   ");
	Serial.println(l_event.param_e);

	return false;
}
