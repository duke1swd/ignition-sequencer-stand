/*
 * This code sends binary data to the daq using the daq opto lines.
 */


void send_som();
void send_eom();
void send_byte(unsigned char m);
void send_long(unsigned long l);
