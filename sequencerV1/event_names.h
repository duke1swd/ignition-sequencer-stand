#include "avr/pgmspace.h"

#define	EVENT_MAX_CODE_LENGTH	64

/*
 * Names used when printing the events.
 */
//                                   xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
static const char ss_00[] PROGMEM = "No Event";
static const char ss_01[] PROGMEM = "Igniter chamber pressure sensor failed";
static const char ss_02[] PROGMEM = "Main chamber pressure sensor failed";
static const char ss_03[] PROGMEM = "t=0";
static const char ss_04[] PROGMEM = "Turn on spark";
static const char ss_05[] PROGMEM = "Open Ig N2O valve";
static const char ss_06[] PROGMEM = "Open Ig IPA valve";
static const char ss_07[] PROGMEM = "Set main valves to +1 degrees";
static const char ss_08[] PROGMEM = "Operator aborted by pressing a button";
static const char ss_09[] PROGMEM = "Ig chamber pressure now good";
static const char ss_10[] PROGMEM = "Ig chamber pressure now bad";
static const char ss_11[] PROGMEM = "Ig chamber pressure now bad after IgPressStable";
static const char ss_12[] PROGMEM = "Ig pressure stable";
static const char ss_13[] PROGMEM = "Turn off spark";
static const char ss_14[] PROGMEM = "No IgPressStable by t=500";
static const char ss_15[] PROGMEM = "Ig chamber pressure has been OK for 20 ms.  t=m";
static const char ss_16[] PROGMEM = "Ig chamber pressure bad after t=m";
static const char ss_17[] PROGMEM = "Main IPA valve commanded to partially open";
static const char ss_18[] PROGMEM = "Main N2O valve commanded to partially open";
static const char ss_19[] PROGMEM = "Main chamber pressure now good";
static const char ss_20[] PROGMEM = "Main chamber pressure now bad";
static const char ss_21[] PROGMEM = "Main chamber pressure not stable by t=m+400";
static const char ss_22[] PROGMEM = "Main valves commanded to full open";
static const char ss_23[] PROGMEM = "IgN2O valve closed";
static const char ss_24[] PROGMEM = "IgIPA valve closed";
static const char ss_25[] PROGMEM = "Sequence complete";

static const char * const event_code_names[] PROGMEM = {
		ss_00,
		ss_01,
		ss_02,
		ss_03,
		ss_04,
		ss_05,
		ss_06,
		ss_07,
		ss_08,
		ss_09,
		ss_10,
		ss_11,
		ss_12,
		ss_13,
		ss_14,
		ss_15,
		ss_16,
		ss_17,
		ss_18,
		ss_19,
		ss_20,
		ss_21,
		ss_22,
		ss_23,
		ss_24,
		ss_25,
};
