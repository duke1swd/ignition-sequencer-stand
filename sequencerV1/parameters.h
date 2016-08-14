/*
 *  Eventually these are replaced by eprom-based parameters.
 *  Until then, they are compiled in
 */

static const unsigned long spark_period = 5;	// milliseconds.  200 Hz

// open / close positions of main propellant valves, in degrees.

static const int ipa_open = 10;
static const int ipa_close = ipa_open + 90;
static const int n2o_open = 10;
static const int n2o_close = n2o_open + 90;
// these pressure numbers assume a 500 PSI pressure sensor.
static const int min_pressure = 90;	// 0 psi gauge, less margin for error
static const int good_pressure = 126;	// 15 psi gauge
static const int max_ig_pressure = 225;	// 75 psi gauge

// timing of the ignition test
static const int ig_run_time = 1000;	// running time after ignition.  In milliseconds.
static const int ig_ipa_time = 50;	// how long after spark before IPA
static const int ig_n2o_time = 50;	// how long after spark before N2O
static const int ig_pressure_time = 250;// how long after spark before we need ignition
