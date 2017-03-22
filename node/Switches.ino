
//Switch property variables

//Switches Setup
void switches_setup() {
  Serial.println("Switches setup");
  // make the switchbuttons' pin an input:
  pinMode(SWITCH_INPUT_A, INPUT);
  pinMode(SWITCH_INPUT_B, INPUT);
}

//functions here
ControlState getSwitchState() {
  int buttonStateA = digitalRead(SWITCH_INPUT_A);
  int buttonStateB = digitalRead(SWITCH_INPUT_B);

  //TODO: fix this when we have real buttons
  if(buttonStateA == 1 && buttonStateB == 1) {
    return ControlState::ON;
  }
  else if(buttonStateA == 1 && buttonStateB == 0) {
    return ControlState::GEO;
  }
  else if(buttonStateA == 0 && buttonStateB == 1) {
    return ControlState::GEO;
  }
  else {
    return ControlState::OFF;
  }

}

