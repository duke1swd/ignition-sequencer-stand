/*
 * Routines to manage eeprom
 */

#include <Arduino.h>
#include <EEPROM.h>
#include "eepromlocal.h"

/*
 * Check if EEPROM is OK.
 * Returns TRUE if there is a problem.
 */

bool
eeprom_check_and_init()
{
	unsigned int i;

	EEPROM.get(EEPROM_MAGIC, i);

	if (i == 0) {
		EEPROM.put(EEPROM_MAGIC, i);
		return false;
	} else if (i == MY_EEPROM_MAGIC_NUMBER)
		return false;

	Serial.print("Bad EEPROM MAGIC.  Expected ");
	Serial.print(MY_EEPROM_MAGIC_NUMBER);
	Serial.print(" got ");
	Serial.print(i);
	Serial.print("\n");

	return true;
}
