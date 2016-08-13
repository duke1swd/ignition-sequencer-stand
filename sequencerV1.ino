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
 *    - Added a new output type, 'servo'
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

/*
 * State machinery is here.
 *
 * This is where the bulk of the interesting code lies.
 */

void menu_led_on();
void menu_led_off();
void igValveTestEnter();
const struct state *menu_check();
const struct state *igValveTestCheck();
const struct state menu_action_1 = {"MA 1", &menu_led_on, NULL, &menu_check};
const struct state menu_action_2 = {"MA 2", &menu_led_off, NULL, &menu_check};
const struct state igValveTest = { "igValveTest", &igValveTestEnter, NULL, &igValveTestCheck};

/*
 * Main menu
 */
const struct menu_item main_menu_items[] = {
  {
     "Ig Valve Test",
     &igValveTest,
  },
  {
     "Spark Test",
     &menu_action_2
  },
  {
     "Main Valve Test",
     &menu_action_2
  },
  {
     "Remote Echo",
     &menu_action_2
  },
  {
     "Local Igniter",
     &menu_action_2
  },
  {
     "Remote Igniter",
     &menu_action_2
  },
  {
     "Main Sequence",
     &menu_action_2
  },
};

const struct menu main_menu = {
  sizeof (main_menu_items) / sizeof (main_menu_items[0]),  // number of menu items
  main_menu_items
};

const struct state *check_startup()
{
  return tft_menu_machine(&main_menu);
}

/*
 * Ignition valve click-test
 * On entry, clear screen and write message
 */
void igValveTestEnter()
{
	tft.fillScreen(TM_TXT_BKG_COLOR);
	tft.setCursor(0, TM_TXT_OFFSET);
	tft.setTextColor(TM_TXT_FG_COLOR);
	tft.print("this is a long line of text.  Does it wrap?");
 while(i_joystick->current_val == JOY_PRESS);
}

const struct state * igValveTestCheck()
{
	if (i_joystick->current_val == JOY_PRESS) 
		return tft_menu_machine(&main_menu);
	return &igValveTest;
}

/*
 * On entering a menu state we should do something.
 * But  what?
 */
void menu_led_on()
{
	// clear the screen and write something
}

void menu_led_off()
{
}

/*
 * For now, the only thing we do in a menu state is flip back to the menu system.
 */
const struct state *menu_check()
{
  return tft_menu_machine(&main_menu);
}

/*
 * Setup routine:
 *	Set up the TFT display.
 *	Set up the state machine.
 */
void setup() {
  Serial.begin(9600);
  Serial.print("Build ");
  Serial.println(build_str);

  Serial.println("Startup.");
  setup_inputs();
  setup_outputs();
  if (!validate_io()) {
    Serial.println("Invalid IO setup. This represents a potentially serious bug. Halting.");
    while (true);
  }

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
  check_state();
  update_outputs();

}

