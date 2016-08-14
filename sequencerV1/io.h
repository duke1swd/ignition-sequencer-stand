/*
 *  Define the input and outputs for the V1 sequencer.
 */

const unsigned char n_multi_input_ladders = 1;
const unsigned int * const multi_input_ladders[n_multi_input_ladders] = {
	joystick_ladder,
};

const int n_inputs = 8;

struct input inputs[n_inputs] = {
    {
	"joystick",	// name
	3,		// pin
	multi_input,	// normal input mode
	multi_input,	// current input mode
	0,		// analog_th.  Must be 0 if type is multi_input
	0,		// analog_hyst
	0,		// prev_val
	0,		// current_val
	0,		// last_change_t
	0,		// filter_a
	0,		// use multi_input_ladder #0
    },
    {
	"ig pressure",	// name
	2,		// pin
	multi_input,	// normal input mode
	multi_input,	// current input mode
	0,		// analog_th.  Must be >=0 for analog pin
	0,		// analog_hyst
	0,		// prev_val
	0,		// current_val
	0,		// last_change_t
	0,		// filter_a
	0,		// use multi_input_ladder #0
    },
    {
	"push 1",	// name
	24,		// pin
	active_low_pullup,// normal input mode
	active_low_pullup,// current input mode
	-1,		// analog_th	must be < 0 for digital pin
	0,		// analog_hyst	unused
	0,		// prev_val
	0,		// current_val
	0,		// last_change_t
	0,		// filter_a	unused
	0,		// multi_input_ladder	unused
    },
    {
	"push 2",	// name
	25,		// pin
	active_low_pullup,// normal input mode
	active_low_pullup,// current input mode
	-1,		// analog_th	must be < 0 for digital pin
	0,		// analog_hyst	unused
	0,		// prev_val
	0,		// current_val
	0,		// last_change_t
	0,		// filter_a	unused
	0,		// multi_input_ladder	unused
    },
    {
	// True when igniter has been safed.
	"safe igniter",	// name
	22,		// pin
	active_low_pullup,// normal input mode
	active_low_pullup,// current input mode
	-1,		// analog_th	must be < 0 for digital pin
	0,		// analog_hyst	unused
	0,		// prev_val
	0,		// current_val
	0,		// last_change_t
	0,		// filter_a	unused
	0,		// multi_input_ladder	unused
    },
    {
	// True when main has been safed.
	"safe main",	// name
	23,		// pin
	active_high_pullup,// normal input mode
	active_high_pullup,// current input mode
	-1,		// analog_th	must be < 0 for digital pin
	0,		// analog_hyst	unused
	0,		// prev_val
	0,		// current_val
	0,		// last_change_t
	0,		// filter_a	unused
	0,		// multi_input_ladder	unused
    },
    {
	"cmd 1",	// name
	28,		// pin
	active_high_in,	// normal input mode
	active_high_in,	// current input mode
	-1,		// analog_th	must be < 0 for digital pin
	0,		// analog_hyst	unused
	0,		// prev_val
	0,		// current_val
	0,		// last_change_t
	0,		// filter_a	unused
	0,		// multi_input_ladder	unused
    },
    {
	"cmd 2",	// name
	29,		// pin
	active_high_in,	// normal input mode
	active_high_in,	// current input mode
	-1,		// analog_th	must be < 0 for digital pin
	0,		// analog_hyst	unused
	0,		// prev_val
	0,		// current_val
	0,		// last_change_t
	0,		// filter_a	unused
	0,		// multi_input_ladder	unused
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

const int n_outputs = 2;

struct output outputs[n_outputs] = {
    {
    	"IPA Ig Valve",	// name
	3,		// pin
	active_high_out,// normal output mode
	active_high_out,// current output mode
	off,		// current state
	0,		// last_change_t
	0,		// pulse_t
    },
    {
    	"N2O Ig Valve",	// name
	4,		// pin
	active_high_out,// normal output mode
	active_high_out,// current output mode
	off,		// current state
	0,		// last_change_t
	0,		// pulse_t
    },

};

struct output * o_ipaIgValve = outputs + 0;
struct output * o_n2oIgValve = outputs + 1;
