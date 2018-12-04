/*
 *  Eventually these are replaced by eprom-based parameters.
 *  Until then, they are compiled in
 */

typedef int int16_t;
static const unsigned long spark_period = 25;	// milliseconds.  40 Hz

/*
 * Servo settings, in degrees.
 */
#define	SERVO_CRACK 1
#define SERVO_OPEN 90

static const int ipa_close = 44;
static const int ipa_crack = ipa_close + SERVO_CRACK;
static const int ipa_partial = ipa_close + 33+10;
static const int ipa_open = ipa_close + SERVO_OPEN;

static const int n2o_close = 47 + 15;
static const int n2o_crack = n2o_close + SERVO_CRACK;
static const int n2o_partial = n2o_close + 37+10;
static const int n2o_open = n2o_close + SERVO_OPEN;


/*
 * Pressure Settings
 */
// these pressure numbers assume a 500 PSI pressure sensor operating at 0.5 to 4.5 volts and a 12 bit DAQ
static const int min_pressure = 360;	// 0 psi gage, less margin for error
static const int max_pressure = 3031;	// about 400 PSI
static const int max_idle_pressure = 450;	// 0 psi gage, plus big margin for error
//static const int good_pressure = 508;		// 15 psi gage
static const int good_pressure = 737;		// 50 psi gage
//static const int max_ig_pressure = 901;		// 75 psi gage
static const int max_ig_pressure = 1196;		// 120 psi gage
static const int main_good_pressure = 676;	// 20 PSI gauge
#define	SENSOR_SANE(x) (((int16_t)(x)) > 90 && ((uint16_t)(x)) < ((uint16_t)4000))

static const int pressure_delta_allowed = 35;	// ig pressure can be this much less than main (about 5 PSI).

/*
 * Timings
 */
static const int ig_run_time = 1200;	// running time after ignition.  In milliseconds.
static const int ig_ipa_time = 0;	// when do we start IPA?
static const int ig_n2o_time = 200;	// when do we start N2O?
static const int ig_spark_time = 0;	// when do we start spark?
static const int mv_crack_time = 0;	// when to take up the slack in the main vales.
static const int ig_spark_off_time = 600; // in debug, when to stop spark
static const int ig_pressure_time = 500;// how long after spark before we need ignition
static const int ig_spark_cont_time = 100;// how long after ignition (pressure) do we keep spark going
// note: ig_pressure_grace should be >= ig_spark_cont_time
static const int ig_pressure_grace = 250;// how long after ignition we start looking for no ignition

static const int shutdown_timeout = 500;// wait this long after shutting down

static const int flow_test_time = 3000;	// fixed length flow run

/*
 * Timings used only in main sequence
 */
static const int ig_stable_spark = 20;	// need 20 ms stable running with spark
static const int ig_stable_no_spark = 20; // need 20 ms stble running with no spark
static const int main_IPA_open_time = 0;	// open main IPA 0 ms after igniter OK
static const int main_N2O_open_time = 50;	// open main N2O 40-60 ms after igniter OK
static const int main_stable_time = 20;	// main chamber pressure to be up and stable for 20 milliseconds.
static const int main_pressure_time = 550; // main pressure to be stable at M+550
static const int main_ig_n2o_close = 50; // turn off igniter n2o after main up.
static const int main_run_time = 8000;	// 8 second running time.

/*
 * Misc
 */
static const float power_volts_per_count = 1./280.06;
