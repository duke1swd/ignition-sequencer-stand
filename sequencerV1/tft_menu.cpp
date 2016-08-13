/*
 *  This code implements a simple menu system on the TFT display.
 *  Operator uses the joystick to scroll up and down, then selects one entry.
 *  When the joystick is pressed, a state, chosen by the menu, is entered.
 */

#include "state_machine.h"
#include "joystick.h"
#include "tft_menu.h"
#include "io_ref.h"
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library

/*
 * Menu state
 */
static unsigned char menu_state;
static const struct menu * current_menu;

/*
 * This machine has three states, one for when a joystick button is pressed, and one for
 * when the joystick button is not pressed.
 * The third state is a secondary idle state, used only if we enter the menu system with
 * the joystick button already in a non-idle state.
 */
const struct state * check_j_idle();
const struct state * check_j_scroll();
const struct state * check_j_wait();
void joystick_display();

static struct state joystick_idle = {
	"jstk idle",
	NULL,			// Enter
	NULL,			// Exit
	check_j_idle,		// Check
};

static struct state joystick_scroll = {
	"jstk scroll",
	joystick_display,	// Enter
	NULL,			// Exit
	check_j_scroll,	// Check
};

static struct state joystick_wait = {
	"jstk wait",
	NULL,
	NULL,
	check_j_wait,
};

/*
 * Paint the TFT display with the current menu
 * This is the workhorse function of this system.
 */
static void screen_paint()
{
	unsigned char i;
	unsigned char row;
	unsigned char start_row;
	int y;
	unsigned char n_items = current_menu->n_items;
	int txt_color;
	extern Adafruit_ST7735 tft;

	start_row = 0;
	if (n_items > TM_N_ROWS && TM_N_ROWS/2 < menu_state) {
		start_row = menu_state - TM_N_ROWS/2;
		if (start_row + TM_N_ROWS > n_items)
			start_row = n_items - TM_N_ROWS;
	}
	tft.setTextSize(TM_TXT_SIZE);
	for (i = 0; i < TM_N_ROWS; i++) {
		y = TM_TXT_OFFSET + i * TM_TXT_SPACE;
		tft.fillRect(0, y, TM_AREA_W, TM_TXT_HEIGHT, TM_TXT_BKG_COLOR);	// erase the line
		row = i + start_row;
		if (row < n_items) {
			tft.setCursor(0, y);
			txt_color = TM_TXT_FG_COLOR;
			if (row == menu_state)
				txt_color = TM_TXT_HIGH_COLOR;
			tft.setTextColor(txt_color);
			tft.print(current_menu->items[row].menu_text);
		}
	}
}

struct state * tft_menu_machine(const struct menu *my_menu)
{
	menu_state = 0;
	current_menu = my_menu;
	screen_paint();
	return &joystick_wait;
}

/*
 * Stay in idle until the joystick button is activated.
 * Then transition to the scroll state.
 *
 * If the button is pressed, then transition out of this state machine
 * into whatever state is associated with the menu item.
 */
const struct state * check_j_idle()
{
	unsigned char v;

	v = i_joystick->current_val;

	if (v == JOY_PRESS) 
		return current_menu->items[menu_state].action_state;

	if (v)
		return &joystick_scroll;

	return &joystick_idle;
};

/*
 * Stay in scroll state as long as the joystick is activated
 * Performs a scroll action (maybe) on state entry.
 */
const struct state * check_j_scroll()
{
	if (i_joystick->current_val != JOY_NONE)
		return &joystick_scroll;
	return &joystick_idle;
};

/*
 * Stay in wait state as long as the joystick is activated.
 * But don't do anything.
 */
const struct state * check_j_wait()
{
	if (i_joystick->current_val)
		return &joystick_wait;
	return &joystick_idle;
};

/*
 * On entry to the scroll state, update the menu
 * as needed, and repaint.
 */
void joystick_display()
{
	unsigned char previous_menu_state;

	previous_menu_state = menu_state;
	switch (i_joystick->current_val) {
	    case JOY_UP:
	    	if (menu_state > 0)
			menu_state -= 1;
		break;
	    case JOY_DOWN:
	    	if (menu_state < current_menu->n_items - 1)
			menu_state += 1;
		break;
	    default:
	    	// press was handled elsewhere
		// left and right are ignored here.
      ;
	}
	if (previous_menu_state != menu_state)
		screen_paint();
}
