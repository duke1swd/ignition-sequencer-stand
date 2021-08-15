#include <Arduino.h>
#include "state_machine.h"
#include "io_ref.h"

/*
 * Panic routine.
 * Called when software knows it is broken
 */

void myPanic(const char *msg) {
    Serial.print("PANIC: ");
    Serial.println(msg);
    digitalWrite(o_redStatus->pin, HIGH);
    digitalWrite(o_powerStatus->pin, LOW);
    while (true);
}
