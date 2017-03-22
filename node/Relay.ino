
//relay property variables

//Relay Setup
RelayState relay_state;
bool isRelayOn;

void relay_setup() {
  Serial.println("Relay Setup");
  pinMode(RELAY_OUT, OUTPUT);
  relay_state = RelayState::OFF;
  isRelayOn = false;
  relay_off();
}

//functions here
void control_relay() {
    ControlState state = ControlState::GEO; //TODO: FIX this... getSwitchState();
    if(state == ControlState::ON) {
      relay_on();
    }
    else if(state == ControlState::OFF) {
      relay_off();
    }
    else if(state == ControlState::GEO) {
      //Serial.println("Switch State: Geo");
      if(relay_state == RelayState::ON) {
        relay_on();
      }
      else if (relay_state == RelayState::OFF){
        relay_off();
      }
    }
    else {}  
}

void relay_off() {
  if(isRelayOn) {
    digitalWrite(RELAY_OUT, LOW);
    Serial.println("Turn off relay");
    isRelayOn = false;
  }
}

void relay_on() {
  if(!isRelayOn) {
    digitalWrite(RELAY_OUT, HIGH);
    Serial.println("Turn on relay");
    isRelayOn = true;
  }
  
}

void set_relay_state_off(){
  relay_state = RelayState::OFF;
}

void set_relay_state_on(){
  relay_state = RelayState::ON;
}

RelayState getRelayState() {
  return relay_state;
}

