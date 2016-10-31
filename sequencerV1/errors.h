/*
 * Error codes
 */

#define	NUM_ERRORS 6

const unsigned char error_base = 1;
const unsigned char errorIgTestAborted =	error_base + 0;		// test aborted by operator
const unsigned char errorIgTestSafe =		error_base + 1;		// test aborted by safe switch
const unsigned char errorIgNoPressure =		error_base + 2;		// pressure sensor not working
const unsigned char errorIgNoIg =		error_base + 3;		// no ignition
const unsigned char errorIgOverPressure =	error_base + 4;		// too much pressure in ignition system
const unsigned char errorPressureInsane =	error_base + 5;		// pressure sensor broken or not connected
const unsigned char errorIgTestPower =		error_base + 6;		// test aborted by power switch turning off

extern const struct state * error_state(unsigned char error_code);
