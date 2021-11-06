#include <Arduino.h>
#include "state_machine.h"
#include "io_ref.h"

/*
 * This code manages pressure zero-point for unsealed gauge type pressure sensors.
 */

#define MAX_SAMP 12
#define	SKIP 64
static unsigned char n_samp;
static unsigned int  t_samp_ig;
static unsigned int  t_samp_main;
static unsigned char skip_counter;
unsigned int  zero_ig;
unsigned int  zero_main;

/*
 * Call to discard any info we have about the pressure sensor.
 * Must be called at least once before we use the sensor
 */
void sensorInit()
{
	void sensorZero();

	n_samp = 0;
	t_samp_ig = 0;
	t_samp_main = 0;
	skip_counter = SKIP;
	sensorZero();
}

/*
 * Call to average in another reading.
 */
void sensorZero()
{
	if (n_samp >= MAX_SAMP) {
		return;
	}

	// Only do the sampling every now and then.
	if (skip_counter++ < SKIP) {
		return;
	}
	skip_counter = 0;

	n_samp++;
	t_samp_ig += i_ig_pressure->filter_a;
	zero_ig = t_samp_ig / n_samp;

	t_samp_main += i_main_press->filter_a;
	zero_main = t_samp_main / n_samp;
}
