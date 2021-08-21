/*
 * Test and calibrate servos.
 */

unsigned long loop_start_t;
void mainN2OSet(int v);

/*
 * Setup routine:
 *	Set up the TFT display.
 *	Set up the state machine.
 */
void setup() {
  void mainValveInit();

  Serial.begin(9600);

  Serial.println("Startup.");
  mainValveInit();
}

void loop() {
  long d;
  int ds;
  loop_start_t = millis();

  Serial.print("Enter value in degrees (not zero)");

  do {
	  d = Serial.parseInt();
	  ds = (int) d;
  } while (ds == 0);
  Serial.print("Got ");
  Serial.print(ds);
  Serial.print('\n');
  if (ds < 1 || ds > 275) {
  	Serial.print("Out of range\n");
  } else {
	mainN2OSet(ds);
  }
}
