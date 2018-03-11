/*
 * Where do things live in the EEPROM?
 */

bool eeprom_check_and_init();

#define	MY_EEPROM_MAGIC_NUMBER	11

// locations
#define	EEPROM_MAGIC		0	// 2 bytes
#define	EEPROM_EVENT_SEQN	2	// 2 bytes
#define	EEPROM_EVENT_SIZE	4	// 2 bytes
#define	EEPROM_EVENT_SIZE_2	6	// 2 bytes

#define	EEPROM_EVENT_LOG	128	// 500 * 4 bytes
