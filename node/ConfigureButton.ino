

void ConfigureButton_setup() {
  Serial.println("Configure Button setup");
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(CONFIGRE_BUTTON_PIN, INPUT_PULLUP);
}

void Cofigure_loop() {
  int pinVal = !digitalRead(CONFIGRE_BUTTON_PIN);
  if(getIsConfigured()) {
    if(pinVal) {
      setIsConfigured(false);
      configure_address();
    }
  }
}

