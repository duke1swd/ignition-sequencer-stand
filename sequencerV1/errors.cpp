/*
 * This code displays error messages.
 */

#include "parameters.h"
#include "state_machine.h"
#include "errors.h"
#include "joystick.h"
#include "tft_menu.h"
#include "io_ref.h"
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library

extern Adafruit_ST7735 tft;
extern struct menu main_menu;

static unsigned char error_code;	// local copy of error code
void erEnter();
const struct state *erCheck();
struct state l_error_state = { "error display", &erEnter, NULL, &erCheck};

/*
 * Called by a check routine.  Saves the
 * error number and returns the error state
 * structure
 */
const struct state *error_state(unsigned char code)
{
	error_code = code;
	return &l_error_state;
}

/*
 * When we enter the routine, display the error code
 */
void
erEnter()
{
	// background is RED
	tft.fillScreen(ST7735_RED);
	tft.setTextSize(TM_TXT_SIZE+1);
	tft.setCursor(8, TM_TXT_OFFSET);
	// text is probably white
	tft.setTextColor(TM_TXT_FG_COLOR);
	tft.print("ERROR");
	tft.setTextSize(TM_TXT_SIZE);
	tft.setCursor(20, TM_TXT_HEIGHT+16+TM_TXT_OFFSET);
	tft.print(error_code);
}

const struct state * erCheck()
{
	if (joystick_edge_value == JOY_PRESS)
		return tft_menu_machine(&main_menu);
	return &l_error_state;
}
