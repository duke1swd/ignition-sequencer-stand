/*
 * P2.1 Sequencer.
 * 
 * This Sketch runs on an Arduino Mega with an AdaFruit 1.8" TFT display shield.
 * It controls
 *  - propellant valves for the igniter
 *  - spark plug for the igniter
 *  - propellant valves for the main motor
 * It senses
 *  - various switches and push buttons on the panel
 *  - control inputs from the breakout box
 *  - the joystick on the shield
 *  - a pressure sensor on the igniter chamber
 *  
 *  Control is done using Evan's clever state machine software
 *    The state machine software has been modified
 *    - Added a new input type, 'multi-button' to handle the joystick
 *
 *  Note that this routine does not interleave TFT display features
 *    in the control loop.  This sequencer does not keep
 *    a running display as it works.
 *
 */

/*
 * Revision History:
 * V0.1:
 *    June   30, 2016 -- initial attemps at coding.
 * V0.2:
 *    August  1, 2016 -- starting to add real code.  Inputs, outputs, valve click test.
 * V0.3:
 *    August 12, 2016 -- more pre-hardware code
 * V0.4:
 *    September 17, 2016 -- running on hardware.  Igniter control mostly done.
 * V0.5:
 *    October 4, 2016 -- running on harware.  Igniter local test producing flames
 */

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library

/*
 *  Pin definitions for the TFT board
 */
#define TFT_CS      53
#define TFT_RST     0
#define TFT_DC      49
#define TFT_SCLK    46
#define TFT_MOSI    45

#include "state_machine.h"
#include "tft_menu.h"
#include "joystick.h"

const char * const build_str = "V0.2: 160801";

/*
 * State machine data structures.
 *   This section defines the states, the inputs, and the outputs.
 */
#include "io.h"

/*
 * TFT Display Control
 */
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
//Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);	// use software SPI

#include "eepromlocal.h"

/*
 * State machinery is here.
 *
 * This is where the bulk of the interesting code lies.
 */

extern struct state igValveTest;	// Note: cannot use extern and const both.  Bug in linker?
extern struct state rmEchoTest;
extern struct state localOptoTest;
extern struct state pressureSensorTest;
extern struct state sparkTest;
extern struct state mainValveTest;
extern struct state igLocalTestEntry;
extern struct state igLocalDebugEntry;
extern struct state igRemoteTestEntry;
extern struct state igRemoteDebugEntry;
extern struct state igLongTestEntry;
extern struct state flowTest;
extern struct state sequenceEntry;
extern struct state eventsToSerial;
extern struct state traceToSerial;
extern struct state traceTest;

/*
 * Main menu
 */
const struct menu_item main_menu_items[] = {
  {
     "Main Sequence",
     &sequenceEntry,
  },
  {
     "Dump Events",
     &eventsToSerial,
  },
#ifdef notdef	// trace now accessed from debug command
  {
     "Dump Trace",
     &traceToSerial,
  },
#endif
  {
     "Trace Test",
     &traceTest,
  },
  {
     "Ig Local Debug",
     &igLocalDebugEntry,
  },
  {
     "Ig Remote Debug",
     &igRemoteDebugEntry,
  },
  {
     "Ig Long Test",
     &igLongTestEntry,
  },
  {
     "Flow Testing",
     &flowTest,
  },
  {
     "Ig Valve Test",
     &igValveTest,
  },
  {
     "Spark Test",
     &sparkTest,
  },
  {
     "Pressure Sensor",
     &pressureSensorTest,
  },
  {
     "Main Valve Test",
     &mainValveTest,
  },
  {
     "Remote Echo",
     &rmEchoTest,
  },
  {
     "Local Opto",
     &localOptoTest,
  },
  {
     "Local Igniter",
     &igLocalTestEntry,
  },
  {
     "Remote Igniter",
     &igRemoteTestEntry,
  },
};

struct menu main_menu = {
  sizeof (main_menu_items) / sizeof (main_menu_items[0]),  // number of menu items
  main_menu_items
};

const struct state *check_startup()
{
  return tft_menu_machine(&main_menu);
}

/*
 * This routine handles capturing the joystick only on the edge.
 *
 * The variable joystick_edge_value contains the value of the joystick
 * for exactly one iteration of loop().  After that it reverts to
 * JOY_NONE until the next time something happens to the joystick.
 *
 * If a state's check routine decides on a new state because of the joystick
 * then by the time the new state's check routine is called
 * the joystick edge value will have been reset to JOY_NONE.
 */
static unsigned char joystick_old_value = JOY_NONE;
unsigned char joystick_edge_value;

static void joystick_edge_trigger()
{
	joystick_edge_value = i_joystick->current_val;
	if (joystick_edge_value == joystick_old_value)
		joystick_edge_value = JOY_NONE;
	else
		joystick_old_value = joystick_edge_value;
}

/*
 * Setup routine:
 *	Set up the TFT display.
 *	Set up the state machine.
 */
void setup() {
  void mainValveInit();
  void event_init();
  void trace_init();
  void myPanic(const char *msg);

  Serial.begin(9600);
  Serial.print("Build ");
  Serial.println(build_str);

  Serial.println("Startup.");
  setup_inputs();
  setup_outputs();
  event_init();
  trace_init();
  digitalWrite(o_powerStatus->pin, HIGH);
  if (!validate_io())
    myPanic("Invalid I/O Setup");
  if (eeprom_check_and_init())
    myPanic("Invalid EEPROM Magic Number");
  mainValveInit();

  // status LEDs
  o_powerStatus->cur_state = on;
  o_greenStatus->cur_state = off;
  o_amberStatus->cur_state = on;
  o_redStatus->cur_state = off;

  // initialize the 1.8" TFT screen
  tft.initR(INITR_BLACKTAB);  // initialize a ST7735S chip, black tab
  tft.fillScreen(ST7735_BLACK);
  tft.setRotation(3);  // rotate output to match installed screen orientation

  // note: if using black background, looks OK to start at position 0,0
  tft.setCursor(0, 0);  // note: sets cursor to pixel position, not line number
  tft.setTextColor(ST7735_WHITE);
  tft.setTextWrap(false);

  // Do this last before we kick off the loop.
  Serial.println("For help, type \"?\".");
  Serial.print("Inputs: ");
  Serial.println(n_inputs);
  Serial.print("Outputs: ");
  Serial.println(n_outputs);
  read_inputs();
}

void loop() {
  loop_start_t = millis();

  handle_serial();
  handle_cmd();

  // basic state machine steps
  read_inputs();
  joystick_edge_trigger();
  check_state();
  update_outputs();

}

