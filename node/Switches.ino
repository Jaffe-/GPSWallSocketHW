
//Switch property variables
int switchButtonA = 2; //TODO: Change
int switchButtonB = 3; //TODO: Change

//Switches Setup
void switches_setup() {
  // make the switchbuttons' pin an input:
  pinMode(switchButtonA, INPUT);
  pinMode(switchButtonB, INPUT);
  
  
}

//functions here
switchState getSwitchState() {
  int buttonStateA = digitalRead(switchButtonA);
  int buttonStateB = digitalRead(switchButtonB);

  //TODO: fix this when we have real buttons
  if(buttonStateA == 1 && buttonStateB == 1) {
    return on;
  }
  else if(buttonStateA == 1 && buttonStateB == 0) {
    return geo;
  }
  else if(buttonStateA == 0 && buttonStateB == 1) {
    return geo;
  }
  else {
    return off;
  }

}

