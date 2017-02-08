
//LED property variables

//LED Setup
void LED_setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
}

//functions here
void LED_on() {
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
}

void LED_off() {
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
}

