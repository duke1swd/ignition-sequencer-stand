/*
 * Code to record a trace of a signal.
 * For now this code only works on analog signal pins.
 *
 * Trace is logged to memory, and dumped to EEPROM
 */

#include "Arduino.h"
#include "EEPROM.h"
#include "parameters.h"
#include "state_machine.h"
#include "io_ref.h"
#include "trace.h"
#include "eepromlocal.h"

#define	TRACE_SIZE	100	// must fit in an unsigned char
#define	TRACE_POST	50

static int trace_buffer[TRACE_SIZE];

static unsigned char n_points;
static unsigned char trigger;
static unsigned char insert;
static bool enabled;
static bool triggered;


/*
 * Initialize the trace system.
 */
void trace_init()
{
	enabled = true;
	triggered = false;
	n_points = 0;
	insert = 0;
}


void trace_trigger()
{
	triggered = true;
	trigger = insert;
}

bool trace_done()
{
	return !enabled;
}

void trace_point(int data)
{
	unsigned char delta;

	if (!enabled)
		return;
	
	// record the data
	trace_buffer[insert++] = data;
	if (n_points < TRACE_SIZE)
		n_points++;
	else if (insert >= TRACE_SIZE)
		insert = 0;

	// if we are triggered, stop after awhile
	if (!triggered)
		return;

	if (insert >= trigger)
		delta = insert - trigger;
	else {
		delta = TRACE_SIZE - trigger;
		delta += insert;
	}
	if (delta >= TRACE_POST)
		enabled = false;
}


/*
 * Write the trace to EEPROM
 * Returns the trace sequence number
 */
unsigned int trace_commit() {
	unsigned char i;
	unsigned int seqn;

	if (n_points <= 0 || !triggered)
		return 0xffff;

	// mark the in-eeprom trace as invalid
	i = 0;
	EEPROM.put(EEPROM_TRACE_SIZE_2, i);

	// record the size of the trace buffer
	EEPROM.put(EEPROM_TRACE_SIZE, n_points);
	Serial.print("Writing "); Serial.print(n_points); Serial.print(" trace to EEPROM\n");
	
	// write the trace data
	for (i = 0; i < n_points; i++)
		EEPROM.put(EEPROM_DATA_TRACE + i * sizeof (int),
				trace_buffer[i]);

	// update the trace seqn number in eeprom
	EEPROM.get(EEPROM_TRACE_SEQN, seqn);
	seqn += 1;
	EEPROM.put(EEPROM_TRACE_SEQN, seqn);

	EEPROM.put(EEPROM_TRACE_SIZE_2, n_points);
	EEPROM.put(EEPROM_TRACE_TRIGGER, trigger);
#ifdef TRACE_PIN
	i = TRACE_PIN;
#else // TRACE_PIN
	i = 255;
#endif // TRACE_PIN
	EEPROM.put(EEPROM_TRACE_PIN, i);
	EEPROM.put(EEPROM_TRACE_INSERT, insert);

	tirggered = false;	// avoid double calls here.

	return seqn;
}

/*
 * Write the data trace to Serial.
 * In order to allow this to be interrupted, we write 1 line at a time.
 * Returns false normally.
 * Returns true when there are no more lines to print.
 * Caller is responsible for starting _i_ at zero and incrementing it.
 * If it returns true on i=0, then no trace exists.
 */

bool trace_to_serial(int i) {
	unsigned char n;
	unsigned int seqn;
	int data;

	enabled = false;

	// If i=0, print the header and check valid
	if (i == 0) {
		EEPROM.get(EEPROM_TRACE_SIZE, n_points);
		EEPROM.get(EEPROM_TRACE_SIZE_2, n);
		if (n != n_points) {
			n_points = 0;
			return true;
		}

		EEPROM.get(EEPROM_TRACE_INSERT, insert);
		EEPROM.get(EEPROM_TRACE_TRIGGER, trigger);
		EEPROM.get(EEPROM_TRACE_SEQN, seqn);
		EEPROM.get(EEPROM_TRACE_PIN, n);
		
		Serial.print("Data Trace #: ");
		Serial.print(seqn);
		Serial.print(" of pin ");
		Serial.print(n);
		Serial.print(" has ");
		Serial.print(n_points);
		Serial.print(" data points\n");
		return false;
	}
	i -= 1;

	// If done, reinitialize trace buffer and tell out loop we are done.
	if (i < 0 || i >= n_points || i >= TRACE_SIZE) {
		trace_init();
		return true;
	}

	// else print a trace data point

	// Deal with wrap-around ring-buffer stuff
	//i += (int)insert;
	i = i + insert;
	if (i >= n_points)
		i -= n_points;

	if (i == trigger)
		Serial.print("*** ");
	else
		Serial.print("    ");

	EEPROM.get(EEPROM_DATA_TRACE + i * sizeof (int), data);
	Serial.println(data);

	return false;
}
