/* Overview:
 * This is intended as a general-purpose framework for state machine based industrial controls.
 *
 * In general, we assume there are some control inputs (sensors, push buttons, etc.) and
 * control outputs (mostly relays). There may also be a keypad and LCD screen, and there is
 * a (USB) serial console.
 *
 * The serial console provides a rich debug and test interface. Serial console commands can query
 * the state of the program, including current inputs and outputs, and can override normal input
 * and output settings. This is intended to make it easy to debug hardware problems, do
 * test and development work while ignoring a sensor that hasn't been installed yet, etc.
 *
 * The overall operation of the program is intended as a finite state machine.
 * The framework provides a simple control loop, which polls the inputs, checks what is needed
 * in the current state, and possibly transitions to a new state. States can specify code to be
 * run on entering or leaving the state.
 *
 * The intent is that each state specifies events that happen on entrance and exit, and polls
 * inputs while running. The check_ routine should examine inputs (and timers, etc.), make
 * any logical checks required, and decide what the next state is. Output should only be performed
 * within the enter_ and exit_ functions.
 *
 * (Code clarity will necessarily dictate less than strict adherence to the above, especially in
 * the case of keyboard / LCD io. Adherence should be strict with respect to control outputs,
 * eg valves and motors.)
 *
 * The serial console is interleaved into the main state machine control loop. This provides
 * low latency state machine operation, even during console use. (Large serial output sizes may
 * interfere with low-latency operation.) This also avoids potential complexity and bugs of
 * an interrupt-driven approach. (The arduino library provides an interrupt-based background
 * buffered serial service. Default buffer sizes are 64B, so IO larger than that will
 * block and degrade latency.)
 *
 * LCD output is buffered and interleaved into state machine operation to preserve latency.
 *
 * Typical loop time with small console IO is < 1 ms with low-complexity states and no LCD
 * updates. LCD updates happen one character per loop, and character write time is about 24
 * ms, so loop times are 24-25 ms during LCD updates.
 *
 */

/*
 * Modified 7/1/16 by S. Daniel.
 *   - Added input mode multi_input. Used for a multiple valued input that uses a resistor ladder
 *     on an analog input pin.
 * Modified 8/1/16 by S. Daniel
 *   - Added output mode servo.
 */

#ifndef state_machine_h
#define state_machine_h

#include <Arduino.h>
#include <string.h>

extern const char * const build_str;

#define N_INPUT_MODES 8
enum input_mode {
	def_in,
	force_on,
	force_off,
	active_low_in,
	active_high_in,
	active_low_pullup,
	active_high_pullup,
	multi_input
	};

#define N_INPUT_STATES 2

#define N_OUTPUT_MODES 6
enum output_mode {
	def_out,
	active_low_out,
	active_high_out,
	force_low,
	force_high,
	servo,
};

#define N_OUTPUT_STATES 8
enum output_state {
	on,
	off,
	single_on,
	single_off,
	pulse_on,
	pulse_off,
	pwm,
	servo_controlled,
};

#define ANALOG_FILTER_TIME 16
#define ANALOG_FILTER_SCALE 4

struct input {
  const char* const name;			// max 11 characters
  unsigned char pin;
  enum input_mode normal;
  enum input_mode current;
  int analog_th;		// set to -1 for digital pin, 0 for multi_input, and a value >= 0 for all others.
  int analog_hyst;
  unsigned char prev_val;
  unsigned char current_val;	// this is the digital value of the input pin, debounced, etc.
  unsigned long last_change_t;
  unsigned int filter_a;	// this is the analog value of the input pin, filtered, if analog_th >= 0.
  unsigned char multi_input_ladder; // which multi_input_ladder should we use?
};

struct output {
  const char * const name;
  unsigned char pin;
  enum output_mode normal;
  enum output_mode current;
  enum output_state cur_state;
  unsigned long last_change_t;
  union {
  	unsigned long pulse_t;	// used for blinking/pulsed outputs
	unsigned long servo_pos;// in degrees
  } p;
};

struct state {
  const char * const name;
  void (*enter)();
  void (*exit)();
  const struct state* (*check)();
};

extern const int n_inputs;
extern struct input inputs[];

extern const int n_outputs;
extern struct output outputs[];

extern const unsigned char n_multi_input_ladders;
extern const unsigned int * const multi_input_ladders[];

//basic states
const struct state* check_startup();

extern const struct state startup;
extern const struct state * current_state;

extern const unsigned long debounce_t;
extern const boolean verbose;
extern unsigned long state_enter_t;
extern unsigned long state_end_t;
extern unsigned long loop_start_t;

void input_setup(struct input* in);
void output_setup(struct output* out);
void update_output(struct output* out);
void setup_inputs();
void setup_outputs();
void read_input(struct input* in);
void read_inputs();
void update_outputs();
void check_state();
void handle_serial();
void handle_cmd();
int find_str(const char* s, const char** a, const int n);
boolean validate_io();

#endif
