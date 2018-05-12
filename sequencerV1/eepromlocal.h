/*
 * Where do things live in the EEPROM?
 *
 * Note: there are 4KB of EEPROM available on this board
 */

bool eeprom_check_and_init();

#define	MY_EEPROM_MAGIC_NUMBER	11

// locations
#define	EEPROM_MAGIC		0	// 2 bytes
#define	EEPROM_EVENT_SEQN	2	// 2 bytes
#define	EEPROM_EVENT_SIZE	4	// 2 bytes
#define	EEPROM_EVENT_SIZE_2	6	// 2 bytes

#define	EEPROM_TRACE_SEQN	8	// 2 bytes
#define	EEPROM_TRACE_SIZE	10	// 1 byte
#define	EEPROM_TRACE_SIZE_2	11	// 1 byte
#define	EEPROM_TRACE_TRIGGER	12	// 2 bytes
#define	EEPROM_TRACE_INSERT	14	// 2 bytes
#define	EEPROM_TRACE_PIN	16	// 1 byte

#define	EEPROM_EVENT_LOG	128	// 500 * 4 bytes

#define	EEPROM_DATA_TRACE	2128	// 512 bytes

