/*
 *  This code handles the main propellant valves.
 *  Includes the click test.
 *  Run code calls here to open and close the valves.
 */

#include "parameters.h"
#include "state_machine.h"
#include "joystick.h"
#include "tft_menu.h"
#include "io_ref.h"
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <Servo.h>

//#define	TS_HACK			// run the servos in parallel.  Used for testing servo slew rates

extern Adafruit_ST7735 tft;
extern struct menu main_menu;

void mainValveTestEnter();
void mainValveTestExit();
const struct state *mainValveTestCheck();
struct state mainValveTest = { "mainValveTest", &mainValveTestEnter, &mainValveTestExit, &mainValveTestCheck};

static bool valveTestMode;
static bool attached;		// true if the servos are currently attached.

/*
 * Functions to open and close the main valves.
 */
Servo N2OServo;
Servo IPAServo;

static void i_do_attach()
{
	if (!attached) {
		N2OServo.attach(N2OServoPin);
		IPAServo.attach(IPAServoPin);
		attached = true;
	}
}

void mainValvesOff()
{
	if (attached) {
		N2OServo.detach();
		IPAServo.detach();
		attached = false;
	}
}

void mainIPAOpen()
{
	i_do_attach();
	if (valveTestMode) {
		o_daq1->cur_state = on;
		o_testled->cur_state = single_on;
#ifdef TS_HACK
		N2OServo.write(n2o_open);
#endif
	}
	IPAServo.write(ipa_open);
}

void
mainIPACrack()
{
	i_do_attach();
	IPAServo.write(ipa_crack);
}

void
mainIPAPartial()
{
	i_do_attach();
	IPAServo.write(ipa_partial);
}

void mainIPAClose()
{
	i_do_attach();
	if (valveTestMode) {
		o_daq1->cur_state = off;
		o_testled->cur_state = single_on;
#ifdef TS_HACK
		N2OServo.write(n2o_close);
#endif
	}
	IPAServo.write(ipa_close);
}

void mainN2OOpen()
{
	i_do_attach();
	if (valveTestMode) {
		o_daq0->cur_state = on;
		o_testled->cur_state = single_on;
#ifdef TS_HACK
		N2OServo.write(ipa_open);
#endif
	}
	N2OServo.write(n2o_open);
}

void
mainN2OCrack()
{
	i_do_attach();
	N2OServo.write(n2o_crack);
}

void
mainN2OPartial()
{
	i_do_attach();
	N2OServo.write(n2o_partial);
}

void mainN2OClose()
{
	i_do_attach();
	if (valveTestMode) {
		o_daq0->cur_state = off;
		o_testled->cur_state = single_on;
#ifdef TS_HACK
		N2OServo.write(ipa_close);
#endif
	}
	N2OServo.write(n2o_close);
}

/*
 * Init in the valves in closed state
 */
void mainValveInit()
{
	attached = false;
	valveTestMode = false;

	mainN2OClose();
	mainIPAClose();
}

/*
 * local state and previous state of buttons
 * Used to optimize display refresh and to
 * change valve state only on button transitions
 */
static unsigned char ls1;
static unsigned char ls2;
static unsigned char ols1;
static unsigned char ols2;
static unsigned char was_safe;

static bool safe_ok()
{
	return i_safe_ig->current_val == 1 && i_safe_main->current_val == 0;
}

/*
 * Display the button states.
 * See igValveTest for details.
 */
static void mainButtonDisplay()
{
	uint16_t c;

	tft.setTextColor(TM_TXT_FG_COLOR);
	tft.setTextSize(3);

	/*
	 * Need igniter safed, mains not safed, or error
	 */
	if (!safe_ok()) {
		if (was_safe == 0) {
			tft.fillRect(0,96,160,32, ST7735_RED);
			tft.setCursor(12, 100);
			tft.print("SAFE ERR");
			was_safe = 1;
		}
		return;
	}

	if (was_safe) {
		tft.fillRect(64, 96, 32, 32, TM_TXT_BKG_COLOR);
		was_safe = 0;
		ols1 = ols2 = 2;
	}

	/*
	 * Else display the input state
	 */
	if (ls1 != ols1) {
		// Erase the display area to correct color.
		// parameters are x,y of upper left, follwed by width and height
		// then color
		c = ls1? ST7735_RED: TM_TXT_BKG_COLOR;
		tft.fillRect(0, 96, 64, 32, c);	// erase the display spot
		tft.setCursor(4, 100);
		tft.print("IPA");
		ols1 = ls1;
	}

	if (ls2 != ols2) {
		// Erase the display area to correct color.
		// parameters are x,y of upper left, follwed by width and height
		// then color
		c = ls2? ST7735_RED: TM_TXT_BKG_COLOR;
		tft.fillRect(96, 96, 64, 32, c);	// erase the display spot
		tft.setCursor(100, 100);
		tft.print("N2O");
		ols2 = ls2;
	}
}

/*
 * On exit make sure both valves are closed
 */
void mainValveTestExit()
{
	mainN2OClose();
	mainIPAClose();
	valveTestMode = false;
}

/*
 * Ignition valve click-test
 * On entry, clear screen and write message
 */
void mainValveTestEnter()
{
	tft.fillScreen(TM_TXT_BKG_COLOR);
	tft.setTextSize(TM_TXT_SIZE+1);
	tft.setCursor(2, TM_TXT_OFFSET);
	tft.setTextColor(TM_TXT_FG_COLOR);
	tft.print("MAIN Valve");
	tft.setTextSize(TM_TXT_SIZE);
	tft.setCursor(20, TM_TXT_HEIGHT+16+TM_TXT_OFFSET);
	tft.setTextColor(TM_TXT_HIGH_COLOR);
	tft.print("Click Test");
	// force the display routine to refresh to state "off"
	ols1 = 1;
	ls1 = 0;
	ols2 = 1;
	ls2 = 0;
	was_safe = 0;
	mainButtonDisplay();
	valveTestMode = true;	// set true before exit to make sure DAQ lines are low on entry.
	mainValveTestExit();	// entry and exit conditions are the same
	valveTestMode = true;	// exit routine sets this false.
}

/*
 * The state machine calls this once per loop().
 * If the joystick has been pressed, then leave the test.
 * Otherwise copy the input buttons to the outputs.
 */
const struct state * mainValveTestCheck()
{
	if (joystick_edge_value == JOY_PRESS)
		return tft_menu_machine(&main_menu);

	if (safe_ok()) {
		ls1 = (i_push_1->current_val || i_cmd_1->current_val);
		ls2 = (i_push_2->current_val || i_cmd_2->current_val);
		if (ls1 && !ols1)
			mainIPAOpen();
		if (ls2 && !ols2)
			mainN2OOpen();
		if (!ls1 && ols1)
			mainIPAClose();
		if (!ls2 && ols2)
			mainN2OClose();
	}
	mainButtonDisplay();

	return &mainValveTest;
}
