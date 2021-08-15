/*
 * This code sends binary data to the daq using the daq opto lines.
 */

#include <Arduino.h>
#include "io_ref.h"
#include "state_machine.h"
#include "sendtodaq.h"

static void send_start();
static void send_end();
static void send8(unsigned char b);
static void set_lines(unsigned char b);

static unsigned char p0, p1;

void send_som()
{
	p0 = o_daq0->pin;
	p1 = o_daq1->pin;
	send_byte(01);
}

void send_eom()
{
	send_byte(04);
}

void send_byte(unsigned char m)
{
	send_start();
	set_lines(0);
	send8(m);
	send_end();
}

void send_long(unsigned long l)
{
	unsigned char c1, c2, c3, c4;

	c1 = (l >> 24) & 0xff;
	c2 = (l >> 16) & 0xff;
	c3 = (l >>  8) & 0xff;
	c4 =  l        & 0xff;
	send_start();
	set_lines(3);
	send8(c1);
	send8(c2);
	send8(c3);
	send8(c4);
	send_end();
}

/*
 *  Set the DAQ lines using the low order 2 bits of the parameter.
 */
static void set_lines(unsigned char b)
{
	if (b & 2)
		digitalWrite(p1, HIGH);
	else
		digitalWrite(p1, LOW);
	if (b & 1)
		digitalWrite(p0, HIGH);
	else
		digitalWrite(p0, LOW);
	delay(1);
}

static void send_start()
{
	set_lines(3);
	set_lines(2);
	set_lines(1);
}

static void send_end()
{
	set_lines(0);
}

static void send8(unsigned char c)
{
	set_lines((c >> 6) & 3);
	set_lines((c >> 4) & 3);
	set_lines((c >> 2) & 3);
	set_lines( c       & 3);
}
