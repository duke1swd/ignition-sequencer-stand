/*
 * Stuff used to manage pressure sensors.
 */

extern unsigned int  zero_ig;
extern unsigned int  zero_main;
extern unsigned char ig_valid;
extern unsigned char main_valid;

#define P_SLOPE_IG	105
#define	P_SLOPE_MAIN	105
#define	PC_SCALE	16

// If the ig pressure is valid, then this is true if ig pressure is less than parameter in gauge PSI.
// p is the raw sensor reading, x is the comparison in psi
#define	IG_PRESSURE_LESS_THAN(p, x)	((p) < zero_ig || (((p) - zero_ig) * PC_SCALE < (x) * P_SLOPE_IG))
#define	MAIN_PRESSURE_LESS_THAN(p, x)	((p) < zero_main || (((p) - zero_main) * PC_SCALE < (x) * P_SLOPE_MAIN))
#define	IG_PRESSURE_VALID(p)		(ig_valid && (p) >= min_pressure && (p) <= max_pressure)
#define	MAIN_PRESSURE_VALID(p)		(main_valid && (p) >= min_pressure && (p) <= max_pressure)

/*
 * Pressure Settings
 */
// these pressure numbers assume a 500 PSI pressure sensor operating at 0.5 to 4.5 volts and a 12 bit DAQ
static const int min_pressure = 360;		// 0 psi gage, less margin for error
static const int max_pressure = 3031;		// about 400 PSI

static const int good_pressure_PSI = 50;
static const int main_good_pressure_PSI = 35;

static const int pressure_delta_allowed = 35;	// ig pressure can be this much less than main (about 5 PSI).
