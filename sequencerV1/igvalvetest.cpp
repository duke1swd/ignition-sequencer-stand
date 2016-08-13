/*
 *  This code handles the ignition valve click test
 */

#include "state_machine.h"
#include "joystick.h"
#include "tft_menu.h"
#include "io_ref.h"
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library

extern Adafruit_ST7735 tft;
extern struct menu main_menu;

void igValveTestEnter();
void igValveTestExit();
const struct state *igValveTestCheck();
struct state igValveTest = { "igValveTest", &igValveTestEnter, &igValveTestExit, &igValveTestCheck};

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
}

/*
 * On exit make sure both valves are closed (off)
 */
void igValveTestExit()
{
}

/*
 * The state machine calls this once per loop().
 * If the joystick has been pressed, then leave the test.
 * Otherwise copy the input buttons to the outputs.
 */
const struct state * igValveTestCheck()
{
	if (joystick_edge_value == JOY_PRESS) 
		return tft_menu_machine(&main_menu);

	o_ipaIgValve->cur_state = i_push_1->current_val? on: off;
	o_n2oIgValve->cur_state = i_push_2->current_val? on: off;

	return &igValveTest;
}
