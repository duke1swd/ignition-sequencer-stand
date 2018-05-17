/*
 *  Define the input and outputs for the V1 sequencer.
 */

const unsigned char n_multi_input_ladders = 1;
const unsigned int * const multi_input_ladders[n_multi_input_ladders] = {
	joystick_ladder,
};

const int n_inputs = 10;

struct input inputs[n_inputs] = {
    {
	"joystick",	// name
	A3,		// pin
	multi_input,	// normal input mode
	multi_input,	// current input mode
	0,		// analog_th.  Must be 0 if type is multi_input
	0,		// analog_hyst
	0,		// prev_val
	0,		// current_val
	no_edge,	// edge
	0,		// last_change_t
	0,		// filter_a
	0,		// use multi_input_ladder #0
    },
    {
	"ig_pressure",	// name
	A2,		// pin
	multi_input,	// normal input mode
	multi_input,	// current input mode
	0,		// analog_th.  Must be >=0 for analog pin
	0,		// analog_hyst
	0,		// prev_val
	0,		// current_val
	no_edge,	// edge
	0,		// last_change_t
	0,		// filter_a
	0,		// use multi_input_ladder #0
    },
    {
	"push_1",	// name
	24,		// pin
	active_low_pullup,// normal input mode
	active_low_pullup,// current input mode
	-1,		// analog_th	must be < 0 for digital pin
	0,		// analog_hyst	unused
	0,		// prev_val
	0,		// current_val
	no_edge,	// edge
	0,		// last_change_t
	0,		// filter_a	unused
	0,		// multi_input_ladder	unused
    },
    {
	"push_2",	// name
	25,		// pin
	active_low_pullup,// normal input mode
	active_low_pullup,// current input mode
	-1,		// analog_th	must be < 0 for digital pin
	0,		// analog_hyst	unused
	0,		// prev_val
	0,		// current_val
	no_edge,	// edge
	0,		// last_change_t
	0,		// filter_a	unused
	0,		// multi_input_ladder	unused
    },
    {
	// True when igniter has been safed.
	"safe_igniter",	// name
	22,		// pin
	active_high_pullup,// normal input mode
	active_high_pullup,// current input mode
	-1,		// analog_th	must be < 0 for digital pin
	0,		// analog_hyst	unused
	0,		// prev_val
	0,		// current_val
	no_edge,	// edge
	0,		// last_change_t
	0,		// filter_a	unused
	0,		// multi_input_ladder	unused
    },
    {
	// True when main has been safed.
	"safe_main",	// name
	23,		// pin
	active_high_pullup,// normal input mode
	active_high_pullup,// current input mode
	-1,		// analog_th	must be < 0 for digital pin
	0,		// analog_hyst	unused
	0,		// prev_val
	0,		// current_val
	no_edge,	// edge
	0,		// last_change_t
	0,		// filter_a	unused
	0,		// multi_input_ladder	unused
    },
    {
	"cmd_1",	// name
	28,		// pin
	active_low_in,	// normal input mode
	active_low_in,	// current input mode
	-1,		// analog_th	must be < 0 for digital pin
	0,		// analog_hyst	unused
	0,		// prev_val
	0,		// current_val
	no_edge,	// edge
	0,		// last_change_t
	0,		// filter_a	unused
	0,		// multi_input_ladder	unused
    },
    {
	"cmd_2",	// name
	29,		// pin
	active_low_in,	// normal input mode
	active_low_in,	// current input mode
	-1,		// analog_th	must be < 0 for digital pin
	0,		// analog_hyst	unused
	0,		// prev_val
	0,		// current_val
	no_edge,	// edge
	0,		// last_change_t
	0,		// filter_a	unused
	0,		// multi_input_ladder	unused
    },
    {
    	"power_sense",	// name
	A5,		// analog P5
	active_high_in,	// normal input mode
	active_high_in,	// current input mode
	840,		// analog_th	trip on main power < 11.9 volts
	0,		// analog_hyst	unused
	0,		// prev_val
	0,		// current_val
	no_edge,	// edge
	0,		// last_change_t
	0,		// filter_a	unused
	0,		// multi_input_ladder	unused
    },
    {
	"main_press",	// name
	A1,		// pin
	multi_input,	// normal input mode
	multi_input,	// current input mode
	0,		// analog_th.  Must be >=0 for analog pin
	0,		// analog_hyst
	0,		// prev_val
	0,		// current_val
	no_edge,	// edge
	0,		// last_change_t
	0,		// filter_a
	0,		// use multi_input_ladder #0
    },
};

struct input * i_joystick = inputs + 0;
struct input * i_ig_pressure = inputs + 1;
struct input * i_push_1 = inputs + 2;
struct input * i_push_2 = inputs + 3;
struct input * i_safe_ig = inputs + 4;
struct input * i_safe_main = inputs + 5;
struct input * i_cmd_1 = inputs + 6;
struct input * i_cmd_2 = inputs + 7;
struct input * i_power_sense = inputs + 8;
struct input * i_main_press = inputs + 9;

const int n_outputs = 10;

struct output outputs[n_outputs] = {
    {
    	"IPA_Ig_Valve",	// name
	3,		// pin
	active_high_out,// normal output mode
	active_high_out,// current output mode
	off,		// current state
	0,		// last_change_t
	0,		// pulse_t
    },
    {
    	"N2O_Ig_Valve",	// name
	4,		// pin
	active_high_out,// normal output mode
	active_high_out,// current output mode
	off,		// current state
	0,		// last_change_t
	0,		// pulse_t
    },
    {
    	"GREEN_LED",	// name
	15,		// pin
	active_high_out,// normal output mode
	active_high_out,// current output mode
	off,		// current state
	0,		// last_change_t
	0,		// pulse_t
    },
    {
    	"AMBER_LED",	// name
	17,		// pin
	active_high_out,// normal output mode
	active_high_out,// current output mode
	off,		// current state
	0,		// last_change_t
	0,		// pulse_t
    },
    {
    	"RED_LED",	// name
	16,		// pin
	active_high_out,// normal output mode
	active_high_out,// current output mode
	on,		// current state	turns off once state machine loop starts up
	0,		// last_change_t
	0,		// pulse_t
    },
    {
    	"Power_LED",	// name
	14,		// pin
	active_high_out,// normal output mode
	active_high_out,// current output mode
	off,		// current state	turns green once state machine loop starts up
	0,		// last_change_t
	0,		// pulse_t
    },
    {
    	"Spark",	// name
	9,		// pin
	active_high_out,// normal output mode
	active_high_out,// current output mode
	off,		// current state
	0,		// last_change_t
	0,		// pulse_t
    },
    {
    	"DAQ_0",	// name
	11,		// pin
	active_high_out,// normal output mode
	active_high_out,// current output mode
	off,		// current state
	0,		// last_change_t
	0,		// pulse_t
    },
    {
    	"DAQ_1",	// name
	10,		// pin
	active_high_out,// normal output mode
	active_high_out,// current output mode
	off,		// current state
	0,		// last_change_t
	0,		// pulse_t
    },
    {
    	"TESTLED",	// name
	12,		// pin
	active_high_out,// normal output mode
	active_high_out,// current output mode
	off,		// current state
	0,		// last_change_t
	100,		// pulse_t
    },

};

struct output * o_ipaIgValve = outputs + 0;
struct output * o_n2oIgValve = outputs + 1;
struct output * o_greenStatus = outputs + 2;
struct output * o_amberStatus = outputs + 3;
struct output * o_redStatus = outputs + 4;
struct output * o_powerStatus = outputs + 5;
struct output * o_spark = outputs + 6;
struct output * o_daq0 = outputs + 7;
struct output * o_daq1 = outputs + 8;
struct output * o_testled = outputs + 9;

#define	IPAServoPin	6
#define	N2OServoPin	5
