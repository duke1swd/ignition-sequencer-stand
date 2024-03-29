/*
 * This code contains code specific to the main sequence run.
 *
 * States used in this module
 * 	sequenceEntry
 * 		This state sets things up and waits for the "fire" button. 
 * 		When the button is pressed exits to state sequence.
 * 		When in this state, the "GO" button is echod to DAQ and runs
 * 		the spark.
 *	sequenceIgLight
 *		This state is time based. Turns on igniter propellants and spark
 *	sequenceIgPressure
 *		This state waits for the igniter to come up to pressure,
 *		and waits for stable pressure with spark off
 *	sequenceMainValvesStart
 *		This state partially opens the main valves and waits for
 *		main chamber pressure to come up
 *	sequenceMVFull
 *		Runs the main burn.  Job is to sequence close valves on a schedule
 *	sequence_report
 *		This state dumps the event log to the DAQ.
 */

/*
 * The NOMAINFAIL flag removes the code that aborts if we do not
 * detect pressure in the main chamber with valve partially open.
 * 
 * The NOMAINPARITAL flag skips the sequenceMainValvesStart stater
 * Actually, what it does is shorten this state to 10 ms and set
 * NOMAINFAIL.
 *
 * NOIGTOOLOW removes that check
 */

/* NO MAIN FAIL ENABLED */
#define NOMAINFAIL 1

/* NO MAIN PARTIAL ENABLED */
#define	NOMAINPARTIAL 1

/* NO IG TOO LOW CHECK
#define NOIGTOOLOW 1

#ifdef NOMAINPARTIAL
#ifndef NOMAINFAIL
#define NOMAINFAIL 1
#endif
#endif

/*
 * NOTES on daq output line control
 *
 * There are 4 main phases the motor goes through, named above as
 * sequenceIgLight through sequenceMVFull.  We number these states
 * 1 through 4, and put the low order bit of that number on daq line 1.
 * Daq line 0 is low throughout, unless we exit to an error state.
 * On error daq line 1 is set high.   If the error is restartable
 * then when we come back here daq 0 will be set low again.
 */

//#define	LOCAL_RUN	1	// debugging.  Allows run from pushbutton

#ifdef LOCAL_RUN
#define	I1	i_push_1
#define	I2	i_push_2
#else
#define	I1	i_cmd_1
#define	I2	i_cmd_2
#endif


#include "parameters.h"
#include "state_machine.h"
#include "errors.h"
#include "joystick.h"
#include "tft_menu.h"
#include "io_ref.h"
#include "events.h"
#include "mainvalves.h"
#include "pressure.h"
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library

#define	SEQ_REP_PULSE_WIDTH	10	// width, in ms, of pulse output on both daq lines at end of run.

extern void spark_run();
extern Adafruit_ST7735 tft;
extern struct menu main_menu;

void sequenceEntryEnter();
void sequenceEntryExit();
const struct state *sequenceEntryCheck();
struct state sequenceEntry = { "sequenceEntry", &sequenceEntryEnter, &sequenceEntryExit, &sequenceEntryCheck};

void sequenceIgLightEnter();
void sequenceIgLightExit();
const struct state *sequenceIgLightCheck();
struct state sequenceIgLight = { "sequenceIgLight", &sequenceIgLightEnter, sequenceIgLightExit, &sequenceIgLightCheck};

void sequenceIgPressureEnter();
void sequenceIgPressureExit();
const struct state *sequenceIgPressureCheck();
struct state sequenceIgPressure = { "sequenceIgPressure", &sequenceIgPressureEnter, &sequenceIgPressureExit, &sequenceIgPressureCheck};

void sequenceMainValvesStartEnter();
void sequenceMainValvesStartExit();
const struct state *sequenceMainValvesStartCheck();
struct state sequenceMainValvesStart = { "sequenceMainValvesStart", &sequenceMainValvesStartEnter, &sequenceMainValvesStartExit, &sequenceMainValvesStartCheck};

void sequenceMVFullEnter();
void sequenceMVFullExit();
const struct state *sequenceMVFullCheck();
struct state sequenceMVFull = { "sequenceMVFull", &sequenceMVFullEnter, &sequenceMVFullExit, &sequenceMVFullCheck};

void sequenceReportEnter();
void sequenceReportExit();
const struct state *sequenceReportCheck();
struct state sequenceReport = { "sequenceReport", &sequenceReportEnter, &sequenceReportExit, &sequenceReportCheck};

static bool was_power;	// true if last iteration we displayed the power error message
static bool was_safe;	// true if last iteration we displayed the safe error message
static bool enter_screen_redraw;	// used to force screen redraw

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
	/*xxx*/return true;
	return i_power_sense->current_val == 1;
}

/*
 * check for abort conditions
 * Used by multiple states.
 */
const struct state *
allAborts()
{
	unsigned int p;

	// Operator aborts
	if (joystick_edge_value == JOY_PRESS ||
			I2->edge == rising ||
#ifndef LOCAL_RUN
			i_push_2->current_val ||
#endif
			i_push_1->current_val) {
		I2->edge = no_edge;
		event(OpAbort, 1);
		return error_state(errorSeqOpAbort);
	}

	// operator abort by safe switch
	if (!safe_ok()) {
		event(OpAbort, 2);
		return error_state(errorSeqSafe);
	}

	// operator abort by power switch
	if (!power_ok()) {
		event(OpAbort, 3);
		return error_state(errorSeqPower);
	}

	// p is the filtered pressure (counts * 4)
	p = i_ig_pressure->filter_a;

	if (!ig_valid) {
		event(IgPressFail, p);
		return error_state(errorIgPressureInsane, p);
	}

	// abort if pressure sensor broken
	if (!IG_PRESSURE_VALID(p)) {
		event(IgPressFail, p);
		return error_state(errorIgNoPressure, p);
	}

	// p is the filtered pressure (counts * 4)
	p = i_main_press->filter_a;

	if (!main_valid) {
		event(MainPressFail, p);
		return error_state(errorMainPressureInsane, p);
	}

	// abort if pressure sensor broken
	if (!MAIN_PRESSURE_VALID(p)) {
/*xxx*/Serial.print("MAIN ABORT:  p=");Serial.print(p);Serial.print("  min_pressure=");Serial.println(min_pressure);
		event(MainPressFail, p);
		return error_state(errorMainNoPressure, p);
	}

	return NULL;
}

/*
 * On Sequence Entry, set up the display and the LEDs
 */
void tft_print_seconds(int t)
{
	int v;

	v = t / 1000;
	t = t - v * 1000;
	tft.print((char)('0' + (char)v));
	tft.print('.');

	v = (t + 50) / 100;
	tft.print((char)('0' + (char)v));
}

void sequenceEntryEnter()
{
	mainValvesOff();			// turn off the servos
	tft.fillScreen(TM_TXT_BKG_COLOR);
	tft.setTextSize(TM_TXT_SIZE+1);
	tft.setCursor(8, TM_TXT_OFFSET);
	tft.setTextColor(TM_TXT_HIGH_COLOR);
	tft.print(F("Main Sequence"));
	tft.setTextSize(TM_TXT_SIZE);
	tft.setCursor(20, TM_TXT_HEIGHT+16+TM_TXT_OFFSET);
	tft.setTextColor(TM_TXT_FG_COLOR);
	tft.print(F("RUN"));
	tft.setTextColor(TM_TXT_HIGH_COLOR);
	tft.print(F(" = "));
	tft_print_seconds(main_run_time);

#ifdef LOCAL_RUN
	tft.setCursor(20, TM_TXT_HEIGHT+34+TM_TXT_OFFSET);
	tft.setTextColor(TM_TXT_HIGH_COLOR);
	tft.print(F("LOCAL RUN"));
#endif
	
	enter_screen_redraw = true;	// force screen redraw
	was_safe = false;
	was_power = false;
	I2->edge = no_edge;

	error_set_restart(&sequenceEntry);
	error_set_restartable(true);
	event_init();
}

/*
 * On SequenceEnter exit erase the screen and set the LEDs to RED.
 * Note that on entrance to a good state we'll reset the LEDs
 * to a more pleasing combination.
 */
void sequenceEntryExit() {
	o_greenStatus->cur_state = off;
	o_amberStatus->cur_state = off;
	o_redStatus->cur_state = on;
	o_daq0->cur_state = off;
	o_daq1->cur_state = off;
	o_ipaIgValve->cur_state = off;
	o_n2oIgValve->cur_state = off;
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

	if (joystick_edge_value == JOY_PRESS)
		return tft_menu_machine(&main_menu);

	// copy the command input button to the daq.
	// This allows remote daq start.
	o_daq0->cur_state = (I1->current_val == 1? on: off);

	// run spark while "GO" button is held
	if (I1->current_val == 1)
		spark_run();

	// until proven otherwise, LEDs indicate not-ready
	o_greenStatus->cur_state = off;
	o_amberStatus->cur_state = off;
	o_redStatus->cur_state = on;

	p = i_ig_pressure->filter_a;
	if (!ig_valid)
		return error_state(errorIgPressureInsane, p);

	if (!IG_PRESSURE_VALID(p))
		return error_state(errorIgNoPressure, p);

	p = i_main_press->filter_a;
	if (!main_valid)
		return error_state(errorMainPressureInsane, p);

	if (!MAIN_PRESSURE_VALID(p)) {
/*xxx*/Serial.print("MAIN ABORT 2:  p=");Serial.print(p);Serial.print("  min_pressure=");Serial.print(min_pressure);
/*xxx*/Serial.print("  i_main_press->filter_a = ");Serial.println(i_main_press->filter_a);
/*xxx*/Serial.print("  pin = ");Serial.println(i_main_press->pin);
		return error_state(errorMainNoPressure, p);
	}

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
			tft.print(F("NO POWER"));
			was_power = true;
		}
		return (current_state);
	}

	if (!safe_ok()) {
		if (!was_safe) {
			tft.fillRect(0,96,160,32, ST7735_RED);
			tft.setCursor(12, 100);
			tft.setTextColor(ST7735_WHITE);
			tft.print(F("SAFE ERR"));
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

	// erase the error and replace with a green rectangle
	if (was_safe || was_power || enter_screen_redraw) {
#ifdef LOCAL_RUN
		tft.fillRect(0,96,160,32, ST7735_BLUE);
#else
		tft.fillRect(0,96,160,32, ST7735_GREEN);
#endif
		was_safe = false;
		was_power = false;
		enter_screen_redraw = false;
	}

	// If the 'fire' button pressed, then it is time to go.
	if (I2->current_val) {
		event_enable();
		event(IgZero, zero_ig);
		event(MainZero, zero_main);
		I2->edge = no_edge; // sequence will abort of rising edge of I2
		return &sequenceIgLight;
	}

	return (current_state);
}

/*
 * State sequenceIgLight
 *
 * This state brings up the propellants and the spark.
 * Once all are running normally, exit to IgPress
 */
unsigned long sequence_time;
unsigned long sequence_phase_time;

static bool ig_ipa_on;
static bool ig_n2o_on;
static bool ig_spark_on;
static bool mv_cracked;
static bool light_enter;

// called when the sequence is commanded to start
void
sequenceIgLightEnter()
{
	/*
	 * This call takes about 100 ms, which is annoying
	 * Net effect is that ignition is delayed by this amount.
	 */
	tft.fillScreen(TM_TXT_BKG_COLOR);

	o_greenStatus->cur_state = pulse_on;	// set to blinking green
	o_amberStatus->cur_state = on;
	o_redStatus->cur_state = off;
	o_daq0->cur_state = off;
	o_daq1->cur_state = on;			// state #1, odd, daq1 is on.
	ig_ipa_on = false;
	ig_n2o_on = false;
	ig_spark_on = false;
	o_ipaIgValve->cur_state = off;
	o_n2oIgValve->cur_state = off;
	mv_cracked = false;
	light_enter = true;
}

void
sequenceIgLightExit()
{
	o_daq0->cur_state = on;			// Set daq0 on error exit.
	o_ipaIgValve->cur_state = off;
	o_n2oIgValve->cur_state = off;
}

const struct state *sequenceIgLightCheck()
{
	unsigned long t;
	const struct state *es;

	es = allAborts();
	if (es)
		return es;

	if (light_enter) {
		/*
		 * Deferred entry code
		 * This code is moved out of the entry routine to ensure a reasonable loop_start_t
		 * Otherwise, the screen erase runs for 100 ms and loop_start_t is 100 ms out of date.
		 */
		event(IgStart, 0);
		sequence_time = loop_start_t;
		sequence_phase_time = loop_start_t;
		light_enter = false;

		/*
		 * Valve are already closed.  These calls ensure the servos are attached 
		 * and powered up.
		 */
		mainIPAClose();
		mainN2OClose();
		return current_state;
	}

	// how long have we been in this state?
	t = loop_start_t - sequence_phase_time;

	// Time to crack the main valves a bit?
	if (!mv_cracked && t >= mv_crack_time) {
		mv_cracked = true;
		event(MvSlack, 0);
		mainIPACrack();
		mainN2OCrack();
	}

	// turn on the igniter IPA valve?
	if (!ig_ipa_on && t >= ig_ipa_time) {
		ig_ipa_on = true;
		event(IgIPA, 0);
		o_ipaIgValve->cur_state = on;
	}
	
	// turn on the igniter N2O valve?
	if (!ig_n2o_on && t >= ig_n2o_time) {
		ig_n2o_on = true;
		event(IgN2O, 0);
		o_n2oIgValve->cur_state = on;
	}
	
	// turn on the spark?
	if (t >= ig_spark_time) {
		if (!ig_spark_on) {
			ig_spark_on = true;
			event(IgSpark, 0);
		}
		spark_run();
	}

	if (ig_ipa_on && ig_n2o_on && ig_spark_on)
		return &sequenceIgPressure;

	return current_state;
}

/*
 * Wait for the igniter to pressurize.
 * Then turn off the spark and verify that it stays at pressure
 */
static unsigned long pressstate_time;
static unsigned long time_M;

static enum pressstate_e {
	pressNoPress,
	pressWaitSpark,
	pressWaitNoSpark,
} pressstate;

void
sequenceIgPressureEnter()
{
	o_daq0->cur_state = off;
	o_daq1->cur_state = off;		// state #2, even, daq1 is off.
	sequence_phase_time = loop_start_t;
	pressstate = pressNoPress;
	o_ipaIgValve->cur_state = on;
	o_n2oIgValve->cur_state = on;
}

void
sequenceIgPressureExit()
{
	o_daq0->cur_state = on;			// Set daq0 on error exit.
	o_ipaIgValve->cur_state = off;
	o_n2oIgValve->cur_state = off;
}

const struct state *
sequenceIgPressureCheck()
{
	unsigned int p;
	const struct state *es;
	bool pressGood;

	es = allAborts();
	if (es)
		return es;
	
	// p is the filtered pressure (counts * 4)
	p = i_ig_pressure->filter_a;
	pressGood = !IG_PRESSURE_LESS_THAN(p, good_pressure_PSI);

	// process transitions of ig pressure above/below threshold.
	// Also handles spark
	switch(pressstate) {
		case pressNoPress:
			spark_run();
			if (pressGood) {
				event(IgPressOK, p);
				pressstate_time = loop_start_t;
				pressstate = pressWaitSpark;
			}
			break;

		case pressWaitSpark:
			spark_run();
			if (!pressGood) {
				event(IgPressNAK, p);
				pressstate_time = loop_start_t;
				pressstate = pressNoPress;
			} else if (loop_start_t - pressstate_time >= ig_stable_spark) {
				event(IgPressStable, p);
				event(IgSparkOff, 0);
				pressstate_time = loop_start_t;
				pressstate = pressWaitNoSpark;
			}
			break;

		case pressWaitNoSpark:
			if (!pressGood) {
				event(IgPressNAK, p);
				event(IgFail0, 0);
				return error_state(errorIgFlameOut);
			} else if (loop_start_t - pressstate_time >= ig_stable_no_spark) {
				time_M = loop_start_t;
				event(IgStable, p);
				return &sequenceMainValvesStart;
			}
			break;
	}
	
	// if the igniter doesn't fire and stabilize within 500 ms, give up.
	if (loop_start_t - sequence_phase_time > ig_pressure_time) {
		event(IgFail1, p);
		return error_state(errorIgNoIg);
	}

	return current_state;
}

static bool closeMainOnExit;
static bool mainIPAIsOpen;
static bool mainN2OIsOpen;
static bool mainPressWasGood;

void
sequenceMainValvesStartEnter()
{
	o_daq0->cur_state = off;
	o_daq1->cur_state = on;			// state #3, odd, daq1 is on.
	sequence_phase_time = loop_start_t;
	closeMainOnExit = true;
	mainIPAIsOpen = false;
	mainN2OIsOpen = false;
	pressstate_time = loop_start_t;
	mainPressWasGood = false;
	o_ipaIgValve->cur_state = on;
	o_n2oIgValve->cur_state = on;
}

void
sequenceMainValvesStartExit()
{
	o_daq0->cur_state = on;			// Set daq0 on error exit.
	o_ipaIgValve->cur_state = off;
	o_n2oIgValve->cur_state = off;
	if (closeMainOnExit) {
		mainIPAClose();
		mainN2OClose();
	}
}

const struct state *
sequenceMainValvesStartCheck()
{
	unsigned int t;
	unsigned int p;
	const struct state *es;

	es = allAborts();
	if (es)
		return es;

	p = i_ig_pressure->filter_a;
	if (IG_PRESSURE_LESS_THAN(p, good_pressure_PSI)) {
		event(IgFail2, p);
		return error_state(errorIgFlameOut, p);
	}

	t = loop_start_t - pressstate_time;
	p = i_main_press->filter_a;
	if (MAIN_PRESSURE_LESS_THAN(p, main_good_pressure_PSI)) {
		if (mainPressWasGood) {
			event(MainPartialNAK, p);
			mainPressWasGood = false;
		}
	} else {
		if (mainPressWasGood) {
			if (t >= main_stable_time) {
				// Success!
				closeMainOnExit = false;
				return &sequenceMVFull;
			}
		} else {
			event(MainPartialOK, p);
			mainPressWasGood = true;
			pressstate_time = loop_start_t;
		}
	}


	// if no success by M+400, give up
	t = loop_start_t - time_M;
	if (t >= main_pressure_time) {
		event(MainFail0, p);
#ifdef NOMAINFAIL
			event(MainPartialOK, p);
			mainPressWasGood = true;
			pressstate_time = loop_start_t;
				closeMainOnExit = false;
				return &sequenceMVFull;
#else
		return error_state(errorSeqNoMain, p);
#endif
	}

	// sequence opening the main valves
	if (!mainIPAIsOpen && t >= main_IPA_open_time) {
		event(MvIPAStart, 0);
		mainIPAPartial();
		mainIPAIsOpen = true;
		error_set_restartable(false);
	}

	if (!mainN2OIsOpen && t >= main_N2O_open_time) {
		event(MvN2OStart, 0);
		mainN2OPartial();
		mainN2OIsOpen = true;
		error_set_restartable(false);
	}
	
	return current_state;
}

static unsigned long full_time;

void 
sequenceMVFullEnter()
{
	o_daq0->cur_state = off;
	o_daq1->cur_state = off;		// state #4, even, daq1 is off.
	full_time = loop_start_t;
	o_ipaIgValve->cur_state = on;
	o_n2oIgValve->cur_state = on;
	error_set_restartable(false);
	event(MvFull, 0);
	mainN2OOpen();
	mainIPAOpen();
}

void 
sequenceMVFullExit()
{
	o_daq0->cur_state = on;			// Set daq0 on error exit.
	o_daq1->cur_state = off;
	event(IgIPAClose, 0);
	event(SequenceDone, 0);
	o_ipaIgValve->cur_state = off;
	o_n2oIgValve->cur_state = off;
	mainIPAClose();
	mainN2OClose();
	o_greenStatus->cur_state = off;
	o_amberStatus->cur_state = pulse_on;
	o_redStatus->cur_state = off;
}

const struct state *
sequenceMVFullCheck()
{
	unsigned long t;
	const struct state *es;
	unsigned int i, m;

	es = allAborts();
	if (es)
		return es;

	// Check that ig pressure is not below chamber pressure.
	// This is complicated because unsigned math and varying zero-points of sensors.
	// Also, need to not abort if both are at zero because we are out of fuel.
	
	// compute calibrated ig
	i = i_ig_pressure->filter_a;
	if (i < zero_ig)
		i = 0;
	else
		i -= zero_ig;
	
	// compute calibrated main
	m = i_main_press->filter_a;
	if (m < zero_main)
		m = 0;
	else
		m -= zero_main;

	// check if ig too much less than main, but don't worry much if main is very low
	if (m < 2 * pressure_delta_allowed)
		m = 2 * pressure_delta_allowed;
	else
		m -= pressure_delta_allowed;

#ifdef NOIGTOOLOW
not reached
	if (i < m) {
		event(IgLessMain, i);
		event(IgLessMain, m);
		return error_state(errorIgTooLow);
	}
#endif

	t = loop_start_t - full_time;

	if (t >= main_ig_n2o_close && ig_n2o_on) {
		o_n2oIgValve->cur_state = off;
		ig_n2o_on = false;
		event(IgN2OClose, 0);
	}

	if (t >= main_run_time)
		return &sequenceReport;

	return current_state;
}

/*
 * This state begins with 10 ms pulse on both DAQ lines,
 * then dumps 10 ms daq off, then dumps the event log
 * to EEPROM.  Finally we send the log sequence number to the DAQ.
 */
static int pulse_state;
static unsigned long seq_rep_next_time;

void 
sequenceReportEnter()
{
	pulse_state = 0;
	seq_rep_next_time = loop_start_t + SEQ_REP_PULSE_WIDTH;
	o_greenStatus->cur_state = off;
	o_amberStatus->cur_state = on;
	o_redStatus->cur_state = off;
	o_daq0->cur_state = on;
	o_daq1->cur_state = on;
}

void 
sequenceReportExit()
{
	o_greenStatus->cur_state = on;
	o_amberStatus->cur_state = off;
	o_redStatus->cur_state = off;
	o_daq0->cur_state = off;
	o_daq1->cur_state = off;
}

/*
 * Pulse DAQ lines, then dump log to EEPROM and exit
 */
const struct state *
sequenceReportCheck()
{
	if (pulse_state == 0) {
		if (loop_start_t >= seq_rep_next_time) {
			pulse_state = 1;
			o_daq0->cur_state = off;
			o_daq1->cur_state = off;
			seq_rep_next_time = loop_start_t + SEQ_REP_PULSE_WIDTH;
		}
		return current_state;
	} else if (pulse_state == 1)  {
		if (loop_start_t >= seq_rep_next_time)
			pulse_state = 2;
		return current_state;
	}

	event_commit_conditional();

	return tft_menu_machine(&main_menu);
}
