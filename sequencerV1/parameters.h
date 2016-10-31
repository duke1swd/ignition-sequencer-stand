/*
 *  Eventually these are replaced by eprom-based parameters.
 *  Until then, they are compiled in
 */

typedef int int16_t;
static const unsigned long spark_period = 5;	// milliseconds.  200 Hz

// open / close positions of main propellant valves, in degrees.

static const int ipa_open = 10;
static const int ipa_close = ipa_open + 90;
static const int n2o_open = 10;
static const int n2o_close = n2o_open + 90;
// these pressure numbers assume a 500 PSI pressure sensor operating at 0.5 to 4.5 volts and a 10 bit DAQ
static const int min_pressure = 383;	// 0 psi gage, less margin for error
static const int max_idle_pressure = 436;	// 0 psi gage, plus margin for error
static const int good_pressure = 508;		// 15 psi gage
//static const int max_ig_pressure = 901;		// 75 psi gage
static const int max_ig_pressure = 1196;		// 120 psi gage
#define	SENSOR_SANE(x) (((int16_t)(x)) > 90 && ((uint16_t)(x)) < ((uint16_t)1250))

// timing of the ignition test
static const int ig_run_time = 1000;	// running time after ignition.  In milliseconds.
static const int ig_ipa_time = 10;	// how long after spark before IPA
static const int ig_n2o_time = 10;	// how long after spark before N2O
static const int ig_pressure_time = 500;// how long after spark before we need ignition
static const int ig_spark_time = 100;	// how long after ignition (pressure) do we keep spark going
// note: ig_pressure_grace should be <= ig_spark_time
static const int ig_pressure_grace = 250;// how long after ignition we start looking for no ignition
