#include "state_machine.h"
#include <Arduino.h>
#include <string.h>
#include "trace.h"

#define INPUT_BUF_SZ 64
char input_buf[INPUT_BUF_SZ];
int input_idx = 0;
boolean input_discard = false;
boolean cmd_valid = false;
const boolean verbose = true;

const unsigned long debounce_t = 25;

char* cmd_str = NULL;
char* id_str = NULL;
char* val_str = NULL;
const char* separator = " \t\r\n";

const char* help_str =
"Valid commands:\n"
"  set_i <input name> <input mode>: set the input to a mode\n"
"  set_om <output name> <output mode>: set the output to a mode\n"
"  set_ov <output name> <output value>: set the output value\n"
"  read <input name>: query the current mode and value of an input\n"
"  reada <input name>: read the analog value of an input\n"
"  read <output name>: query the current mode and value of an output\n"
"  tracedump: dump the current signal trace\n"
"  state: query the current state of the state machine\n"
"  list_io: list the available inputs and outputs\n"
"  list_modes: list available input / output modes\n";

const char* input_mode_str[N_INPUT_MODES] = {
	"def_in",
	"force_on",
	"force_off",
	"active_low_in",
	"active_high_in",
	"active_low_pullup",
	"active_high_pullup",
	"multi_input",
};

const char* input_state_str[] =  {"off", "on"};

const char* output_mode_str[N_OUTPUT_MODES] = {
	"def_out",
	"active_low_out",
	"active_high_out",
	"force_low",
	"force_high",
	"servo",
};

const char* output_state_str[N_OUTPUT_STATES] = {
	"on",
	"off",
	"single_on",
	"single_off",
	"pulse_on",
	"pulse_off"
	"pwm",
	"servo_congtrolled",
};


const struct state startup = {"startup", NULL,          NULL,         &check_startup};

const struct state * current_state = &startup;

unsigned long loop_start_t = 0; //time this loop iteration started. Used as "now" for things that care.
unsigned long state_enter_t = 0;
unsigned long state_end_t = 0;

int find_str(const char* s, const char** a, const int n) {
  int i;
  for (i = 0; i < n; i++) {
    if (strcmp(s, a[i]) == 0) return i;
  }
  return -1;
}

void handle_cmd() {
  if (!cmd_valid) return;
  if (input_discard) return;
  if (verbose) {
    Serial.print("Command: '");
    Serial.print(input_buf);
    Serial.println("'");
  }

  cmd_str = strtok(input_buf, separator);
  id_str = strtok(NULL, separator);
  val_str = strtok(NULL, separator);  
  
  if (false && verbose) {
    Serial.print("Command: '");
    Serial.print((cmd_str != NULL) ? cmd_str : "NULL");
    Serial.print("'\nId:      '");
    Serial.print((id_str != NULL) ? id_str : "NULL");
    Serial.print("'\nVal:     '");
    Serial.print((val_str != NULL) ? val_str : "NULL");
    Serial.print("'\n");
  }
  
  if (cmd_str == NULL) {
    Serial.println("No command found. Type \"?\" for additional help.");
    input_idx = 0;
    cmd_valid = false;
    cmd_str = NULL;
    id_str = NULL;
    val_str = NULL;
    return;
  }

  input* in = NULL;
  output* out = NULL;
  
  for (int i = 0; i < n_inputs; i++) {
    if (strcmp(inputs[i].name, id_str) == 0) {
      in = &inputs[i];
      break;
    }
  }
  
  for (int i = 0; i < n_outputs; i++) {
    if (strcmp(outputs[i].name, id_str) == 0) {
      out = &outputs[i];
      break;
    }
  }
  
  if (false && verbose) {
    if (in != NULL) Serial.println("Matching input found.");
    if (out != NULL) Serial.println("Matching output found.");
  }
  
  if (strncmp(cmd_str, "?", 1) == 0) {
    Serial.println("State machine console interface help.");
    Serial.print("Build: ");
    Serial.println(build_str);
    Serial.println(help_str);
  } else if (strcmp(cmd_str, "read") == 0) {
    if (in != NULL) {
      Serial.print("Normal: ");
      Serial.println(input_mode_str[in->normal]);
      Serial.print("Current: ");
      Serial.println(input_mode_str[in->current]);
      Serial.print("Val: ");
      Serial.println(input_state_str[in->current_val]);
      if (in->analog_th >= 0) {
        Serial.print("Filtered: ");
        Serial.println(in->filter_a);
      }
    }
    if (out != NULL) {
      Serial.print("Output mode normal: ");
      Serial.println(output_mode_str[out->normal]);
      Serial.print("Output mode current: ");
      Serial.println(output_mode_str[out->current]);
      Serial.print("Value: ");
      Serial.println(output_state_str[out->cur_state]);
    }
  } else if (strcmp(cmd_str, "reada") == 0) {
    if (in != NULL) {
      int a = analogRead(in->pin);
      Serial.println(a);
    }
  } else if (strcmp(cmd_str, "set_i") == 0) {
    if (val_str == NULL) {
      Serial.println("No value to set.");
    } else if (in == NULL) {
      Serial.println("No input specified.");
    } else {
      int v = find_str(val_str, input_mode_str, N_INPUT_MODES);
      if (v == -1) {
        Serial.println("Value not valid.");
      } else {
        in->current = (input_mode)v;
      }
    }
  } else if (strcmp(cmd_str, "set_om") == 0) {
    if (val_str == NULL) {
      Serial.println("No value to set.");
    } else if (out == NULL) {
      Serial.println("No output specified.");
    } else {
      int v = find_str(val_str, output_mode_str, N_OUTPUT_MODES);
      if (v == -1) {
        Serial.println("Value not valid.");
      } else {
        out->current = (output_mode)v;
      }      
    }
  } else if (strcmp(cmd_str, "set_ov") == 0) {
    if (val_str == NULL) {
      Serial.println("No value to set.");
    } else if (out == NULL) {
      Serial.println("No output specified.");
    } else {
      int v = find_str(val_str, output_state_str, N_OUTPUT_STATES);
      if (v == -1) {
        Serial.println("Value not valid.");
      } else {
        out->cur_state = (output_state)v;
      }      
    }
 #ifdef TRACE
  } else if (strcmp(cmd_str, "tracedump") == 0) {
    for (int i = 0; !trace_to_serial(i); i++) ;
    Serial.println("Done printing trace");
 #endif
  } else if (strcmp(cmd_str, "state") == 0) {
    Serial.print("Current state: ");
    Serial.println(current_state->name);
  } else if (strcmp(cmd_str, "list_io") == 0) {
    Serial.println("\nAvailable inputs:");
    for (int i = 0; i < n_inputs; i++) Serial.println(inputs[i].name);
    Serial.println("\nAvailable outputs:");
    for (int i = 0; i < n_outputs; i++) Serial.println(outputs[i].name);
  } else if (strcmp(cmd_str, "list_modes") == 0) {
    Serial.println("\nAvailable input modes:");
    for (int i = 0; i < N_INPUT_MODES; i++) Serial.println(input_mode_str[i]);
    Serial.println("\nAvailable output modes:");
    for (int i = 0; i < N_OUTPUT_MODES; i++) Serial.println(output_mode_str[i]);
    Serial.println("\nAvailable output states:");
    for (int i = 0; i < N_OUTPUT_STATES; i++) Serial.println(output_state_str[i]);
  } else {
    Serial.println("No valid command found. Type \"?\" for help.");
  }
  
  input_idx = 0;
  cmd_valid = false;
  cmd_str = NULL;
  id_str = NULL;
  val_str = NULL;
}

//Handle a single character from the serial port. Don't process a bunch at once, for latency / simplicity reasons.
//Precondition: the beginning of the buffer holds less than a complete command.
//This means: No '\n' or '\0' characters at indexes < input_idx.
//Postcondition: if there is a complete command in the buffer, cmd_valid will be set
//Also, input_idx will be left set to the index of the '\0' character, so looping like
// for (int i = 0; i < input_idx; i++);
//will work well.
//Implications: after calling, handle the command if applicable.
//Also: the command handling routine should reset input_idx to 0 when done.
void handle_serial() {
  if (!Serial.available()) return;
  if (input_idx >= INPUT_BUF_SZ) return;
  
  int in = Serial.read();
  if (in == -1) return;
  
  if (in == '\n') {
    if (input_discard) {
      input_idx = 0;
      input_discard = false;
    }
    input_buf[input_idx] = 0;
    cmd_valid = true;
    return;
  }
  if (input_idx >= INPUT_BUF_SZ + 1) {
    //too long, no newline: discard
    Serial.println("Too much input, discarding.");
    input_discard = true;
    return;
  }
  input_buf[input_idx] = in;
  input_idx++;
}


//TODO: check that names and pins are unique
boolean validate_io() {
  unsigned char ipin1;
  unsigned char ipin2;
  const unsigned char analog_pin = 0x80;	// bit to mark a pin as analog

  for (int i = 0; i < n_inputs; i++) {
    ipin1 = inputs[i].pin;
    if (inputs[i].analog_th >= 0) {
      // analog pin
      ipin1 |= analog_pin;
      if ((inputs[i].normal == multi_input || inputs[i].current == multi_input) &&
        inputs[i].analog_th != 0)
	  return false;
    } else if (inputs[i].analog_th == -1) {
      // digital pin
      //if (inputs[i].normal != active_low_in && inputs[i].normal != active_high_in) return false;
      //if (inputs[i].current != active_low_in && inputs[i].current != active_high_in) return false;
    } else {
      return false;
    }
    if (inputs[i].analog_hyst < 0) return false;
    for (int j = i + 1; j < n_inputs; j++) {
      ipin2 = inputs[j].pin;
      if (inputs[j].analog_th >= 0)
      	ipin2 |= analog_pin;
      if (ipin1 == ipin2) return false;
    }
    for (int j = 0; j < n_outputs; j++) {
      if (ipin1 == outputs[j].pin) return false;
    }
  }
  for (int i = 0; i < n_outputs; i++) {
    for (int j = i + 1; j < n_outputs; j++) {
      if (outputs[i].pin == outputs[j].pin) return false;
    }
  }
  return true;
}

void read_inputs() {
  for (int i = 0; i < n_inputs; i++) {
    read_input(&inputs[i]);
  }
}

void update_outputs() {
  for (int i = 0; i < n_outputs; i++) {
    update_output(&outputs[i]);
  }
}

void check_state() {
  void myPanic(const char *msg);
  if (current_state->check != NULL) {
    const struct state* new_state = (*(current_state->check))();
    if (new_state == NULL) myPanic("null state");
    if (new_state != current_state) {
      if (verbose) {
        Serial.print("Leaving ");
        Serial.print(current_state->name);
      }
      state_end_t = loop_start_t;
      if (current_state->exit != NULL) (*(current_state->exit))();
      current_state = new_state;
      state_enter_t = state_end_t;
      if (verbose) {
        Serial.print("; Entering ");
        Serial.println(new_state->name);
      }
      if (current_state->enter != NULL) (*(current_state->enter))();
    }
  }
}

void setup_inputs() {
  for (int i = 0; i < n_inputs; i++) {
    input_setup(&inputs[i]);
  }
}

void setup_outputs() {
  for (int i = 0; i < n_outputs; i++) {
    output_setup(&outputs[i]);
  }
}

void read_input(struct input* in) {
  unsigned char in_val = false;
  input_mode m = in->current;
  if (m == def_in) m = in->normal;
  
  if (m == force_on) {
    in_val = true;
  } else if (m == force_off) {
    in_val = false;
  } else {
    if (in->analog_th == -1) {
      in_val = digitalRead(in->pin);
      if (m == active_low_in || m == active_low_pullup) in_val = !in_val;
    } else {
      int v = analogRead(in->pin);
      unsigned long f = in->filter_a;
      f *= (ANALOG_FILTER_TIME - 1UL);
      f += v * ANALOG_FILTER_SCALE + ANALOG_FILTER_SCALE/2;
      f /= ANALOG_FILTER_TIME;
      in->filter_a = f;
#ifdef TRACE_PIN
      if (in->pin == TRACE_PIN)
	 trace_point((int)f);
#endif // TRACE_PIN
      f /= ANALOG_FILTER_SCALE;
      if (m == active_low_in) {
        if (in->prev_val) {
          in_val = (f < in->analog_th);
        } else {
          in_val = (f < in->analog_th - in->analog_hyst);
        }
      } else if (m == active_high_in) {
        if (in->prev_val) {
          in_val = (f >= in->analog_th - in->analog_hyst);
        } else {
          in_val = (f >= in->analog_th);
        }
      } else if (m == multi_input) {
        const unsigned int *ladder = multi_input_ladders[in->multi_input_ladder];
	unsigned int l = ladder[0];
	for (in_val = 1; in_val <= l; in_val++)
	  if (v < ladder[in_val])
	    goto found;
	in_val = 0;	// off the top of the ladder, return 0.
	found:;
      } else {
        in_val = false; //not a valid analog input configuration
      }
    }
  }
  
  /*
   * Debounce the pins.
   *
   * Note 1: multi_input pin debouncing is good, if what you have is
   * a control with multiple values, e.g. the joystick on the Adafruit
   * TFT display.  It is bad if what you have is a sensor and you want
   * to divide up its range into good, warning, error regions, and trigger
   * state changes on transition to those regions.  This code assumes
   * you have a joystick, not a sensor.
   *
   * Note 2: in this block we use millis() rather than loop_start_t
   * because it may have been some time since loop_start_t before
   * this routine is called.  The pins are sampled in this routine,
   * so using millis() is more accurate.
   */
  if (in_val != in->prev_val) {
    in->prev_val = in_val;
    in->last_change_t = millis();
  } else {
    if (in->last_change_t + debounce_t < millis() &&
      in->current_val != in_val) {
        in->edge = (in_val? rising: falling);
        in->current_val = in_val;
    }
  }
}


void output_setup(struct output* out) {
  update_output(out);
  pinMode(out->pin, OUTPUT);
}

void update_output(output* out) {
  output_mode m = out->current;
  if (m == def_out) m = out->normal;
  if (m == force_low) {
    digitalWrite(out->pin, LOW);
    return;
  } else if (m == force_high) {
    digitalWrite(out->pin, HIGH);
    return;
  } else {
  }
  switch (out->cur_state) {
    case on:
    case off:
      digitalWrite(out->pin, (m != active_low_out) != (out->cur_state != on)); // All the != operators force casts to booleans
      break;
    case single_on:
    case single_off:
      if (out->last_change_t == 0) {
        out->last_change_t = loop_start_t;
      }
      if (out->last_change_t + out->p.pulse_t < loop_start_t) {
        out->cur_state = (out->cur_state == single_on) ? off : on;
        digitalWrite(out->pin, (m != active_low_out) == (out->cur_state != on));
        out->last_change_t = 0;
      } else {
        digitalWrite(out->pin, (m != active_low_out) == (out->cur_state != single_on));
      }
      break;
    case pulse_on:
    case pulse_off:
      if (out->last_change_t + out->p.pulse_t < loop_start_t) {
        out->cur_state = (out->cur_state == pulse_on) ? pulse_off : pulse_on;
        digitalWrite(out->pin, (m != active_low_out) == (out->cur_state != pulse_on));
        out->last_change_t = loop_start_t;
      } else {
        digitalWrite(out->pin, (m != active_low_out) == (out->cur_state != pulse_on));
      }
      break;
    case pwm:
      //not yet supported
      digitalWrite(out->pin, LOW);
      break;
  }
}

void input_setup(struct input* in) {
  input_mode m = in->current;
  if (m == def_in) m = in->normal;
  switch (m) {
    case active_low_pullup:
    case active_high_pullup:
      pinMode(in->pin, INPUT_PULLUP);
      break;
    default:
      pinMode(in->pin, INPUT);
      break;
  }
}

