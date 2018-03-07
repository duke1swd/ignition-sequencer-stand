/*
 * Error codes
 */

#define	NUM_ERRORS 15

const unsigned char error_base = 1;
const unsigned char errorIgTestAborted =	error_base + 0;		// test aborted by operator
const unsigned char errorIgTestSafe =		error_base + 1;		// test aborted by safe switch
const unsigned char errorIgNoPressure =		error_base + 2;		// ig pressure sensor not working
const unsigned char errorIgNoIg =		error_base + 3;		// no ignition
const unsigned char errorIgOverPressure =	error_base + 4;		// too much pressure in ignition system
const unsigned char errorIgPressureInsane =	error_base + 5;		// ig pressure sensor broken or not connected
const unsigned char errorIgTestPower =		error_base + 6;		// test aborted by power switch turning off
const unsigned char errorIgFlameOut =		error_base + 7;		// lost igniter pressure
const unsigned char errorMainPressureInsane =	error_base + 8;		// main pressure sensor broken
const unsigned char errorMainNoPressure =	error_base + 9;		// main pressure sensor value out of range
const unsigned char errorSeqLocalAbort =	error_base + 10;	// local push button at start of main sequence

const unsigned char errorSeqSafe =		error_base + 11;	// sequence aborted by safe switch
const unsigned char errorSeqPower =		error_base + 12;	// sequence aborted by power switch turning off
const unsigned char errorSeqOpAbort =		error_base + 13;	// sequence aborted by operator
const unsigned char errorSeqNoMain =		error_base + 14;	// sequence aborted when main chamber doesn't light

extern const struct state * error_state(unsigned char error_code);
extern void error_set_restart(const struct state *restart_state);
extern void error_set_restartable(bool restartable);
