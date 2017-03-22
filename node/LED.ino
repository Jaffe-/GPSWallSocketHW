
//LED property variables
boolean isOn = false;

//LED Setup
void LED_setup() {
  Serial.println("LEDs setup");
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
}

//functions here
void LED_on() {
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  isOn = true;
}

void LED_off() {
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  isOn = false;
}

//Used for testing timer1 interrupts
void timerIsr() 
{
  //Serial.println("Test");
  if(isOn) {
    LED_off();
  }
  else {
    LED_on();
  }
}

void LED_test() {
  delay(500);
  LED_on();
  delay(500);
  LED_off();
}


