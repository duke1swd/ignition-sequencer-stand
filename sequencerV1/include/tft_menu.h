/*
 * A menu is an array of menu items plus a length.
 * Each menu item is a string and a pointer to a state.
 * 
 * Only one entry point:
 * 	struct state * tft_menu_machine(const struct menu *my_menu)
 * This routine is called at the end of a check function, when a decision has been made
 * to enter the menu state machine.  I.e.:
 * 	return tft_menu_machine(&my_main_menu);
 * Caller must also define an input named "i_joystick".
 *	This is a multi_input that returns 0-5 depending on joystickness.
 */

/*
 * Created 7/1/16 by S. Daniel.
 */

#ifndef tft_menu_h
#define tft_menu_h

#include "state_machine.h"
#include <Adafruit_ST7735.h> // Hardware-specific library

struct menu_item {
	const char * const PROGMEM menu_text;
	const struct state * const action_state;
};

struct menu {
	const unsigned char n_items;
	const struct menu_item * const items;
};

extern struct state * tft_menu_machine(const struct menu *my_menu);

/*
 * This section defines the screen layout.
 */
#define	TM_N_ROWS	3	// number of rows of menu we can display
#define	TM_TXT_SPACE	40	// number of pixels (vertically) between rows
#define	TM_TXT_OFFSET	4	// number of pixels above first row
#define	TM_TXT_SIZE	2	// text size to pass to gfx library
#define	TM_TXT_HEIGHT	(TM_TXT_SIZE * 8)
/* define the area of screen we are actually using */
#define	TM_AREA_H	(TM_TXT_SPACE * (TM_N_ROWS-1) + TM_TXT_SIZE * 8)
#define	TM_AREA_W	160
#define	TM_TXT_BKG_COLOR	ST7735_BLACK
#define	TM_TXT_FG_COLOR		ST7735_BLUE
#define	TM_TXT_HIGH_COLOR	ST7735_WHITE

#endif
