/*
 *  This code handles the main propellant valves.
 *  Includes the click test.
 *  Run code calls here to open and close the valves.
 */
#include "servopins.h"
#include <Servo.h>

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

void mainIPASet(int v)
{
	i_do_attach();
	IPAServo.write(v);
}

void mainN2OSet(int v)
{
	i_do_attach();
	N2OServo.write(v);
}

/*
 * Init in the valves in closed state
 */
void mainValveInit()
{
	attached = false;
}
