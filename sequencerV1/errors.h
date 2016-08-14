/*
 * Error codes
 */

const unsigned char error_base = 1;
const unsigned char errorIgTestAborted =	error_base + 0;		// test aborted by operator
const unsigned char errorIgTestSafe =		error_base + 1;		// test aborted by safe switch
const unsigned char errorIgNoPressure =		error_base + 2;		// pressure sensor not working
const unsigned char errorIgNoIg =		error_base + 3;		// no ignition
const unsigned char errorIgOverPressure =	error_base + 4;		// too much pressure in ignition system

extern struct state * error_state(unsigned char error_code);
