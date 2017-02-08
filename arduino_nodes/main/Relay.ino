
//relay property variables

//Relay Setup


void relay_setup() {

}

//functions here
void control_realy() {
    switchState state = getSwitchState();
    if(state == on) {
      Serial.println("On");
    }
    else if(state == off) {
      Serial.println("Off");
    }
    else if(state == geo) {
      Serial.println("Geo");
    }
    else {}  
  }
