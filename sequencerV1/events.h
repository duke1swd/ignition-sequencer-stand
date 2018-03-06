/*
 * During the main sequence we record a bunch of events.
 * These are later dumped to the DAQ for retrieval.
 *
 * Here are the known events:
 */

#define	EVENT_BUFFER_SIZE	500

enum event_codes {
	no_event,	// Nothing to nobody
	IgPressFail,	// Igniter chamber pressure sensor failed
	MainPressFail,	// Igniter chamber pressure sensor failed
	IgStart,	// t=0
	IgSpark,	// turn on spark
	IgN2O,		// open Ig N2O valve
	IgIPA,		// open Ig IPA valve
	MvSlack,	// set main valves to +1 degrees
	OpAbort,	// operator aborted by pressing a button
	IgPressOK,	// Ig chamber pressure goes from < 70 PSI to > 70 PSI
	IgPressNAK,	// Ig chamber pressure goes from > 70 PSI to < 70 PSI
	IgFail0,	// Ig chamber pressure < 70 PSI after IgPressStable
	IgPressStable,	// Ig pressure deamed stable
	IgSparkOff,	// turn off spark
	IgFail1,	// No IgPressStable by t=500
	IgStable,	// Ig chamber pressure has been OK for 20 ms.  t=m
	IgFail2,	// Ig chamber pressure < 70 PSI after t=m
	MvIPAStart,	// Main IPA valve commanded to partially open
	MvN2OStart,	// Main N2O valve commanded to partially open
	MainPartialOK,	// Main chamber pressure goes from < 50 PSI to > 50 PSI
	MainPartialNAK,	// Main chamber pressure goes from > 50 PSI to < 50 PSI
	MainFail0,	// Main chamber pressure not stable by t=m+400
	MvFull,		// Main valves commanded to full open
	IgN2OClose,	// IgN2O valve closed
	IgIPAClose,	// IgIPA valve closed
	SequenceDone,	// Sequence complete
};

/*
 * Called to record an event.
 * Pass in the event.  loop_start_t is used for the timestamp
 */
void event_init();
void event_enable();
void event_disable();
void event(enum event_codes);
enum event_codes event_code(int);
unsigned long event_time(int);
int event_count();
bool event_to_daq(int i);
