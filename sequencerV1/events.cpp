/*
 * Code to record events.
 * Events are only used by the main sequence.
 *
 * Events are signaled on the DAQ lines.
 * Events are logged to an in memory buffer.
 * Events are written to the DAQ after sequence completes.
 * Errors are signaled on the LEDs.
 */

#include "parameters.h"
#include "state_machine.h"
#include "events.h"
#include "io_ref.h"

static struct event_s {
	enum event_codes e_e;
	unsigned int time_e;
} event_buffer[EVENT_BUFFER_SIZE];

static int n_events;
static bool enabled;


/*
 * Initialize the event system, discarding any events that may be
 */
void event_init()
{
	enabled = false;
	n_events = 0;
}

void event_enable()
{
	enabled = true;
	n_events = 0;
}

void event_disable()
{
	enabled = false;
}

/*
 * Called to record an event.
 * Pass in the event.  loop_start_t is used for the timestamp
 */
void event(enum event_codes e) {
	if (!enabled)
		return;

	// Record the event if we have space
	if (n_events < EVENT_BUFFER_SIZE) {
		event_buffer[n_events].e_e = e;
		event_buffer[n_events].time_e = loop_start_t;
	}

	n_events++;
}

enum event_codes event_code(int i) {
	if (i < 0 || i >= n_events || i >= EVENT_BUFFER_SIZE)
		return no_event;
	return event_buffer[i].e_e;
}

unsigned long event_time(int i) {
	if (i < 0 || i >= n_events || i >= EVENT_BUFFER_SIZE)
		return 0;
	return event_buffer[i].time_e;
}

int event_count() {
	return n_events;
}

/*
 * Write the entire event log to the daq by toggling the daq lines.
 * Returns true on success, false when you've run out of stuff to dump
 */
bool event_to_daq(int i) {
	// QQQ 
	return false;
}
