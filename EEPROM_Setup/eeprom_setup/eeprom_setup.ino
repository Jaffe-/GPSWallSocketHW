#include <EEPROM.h>

/** the current address in the EEPROM (i.e. which byte we're going to write to next) **/
int address = 0;
int val = 255;

void setup() {
  Serial.begin(9600);
  // put your setup code here, to run once:
  Serial.println("Starting EEPROM clearing");

  while(true) {
    EEPROM.update(address, val);
    address = address + 1;
    if (address == EEPROM.length()) {
      break;
    }
    Serial.print("Updating address: ");
    Serial.println(address);
    delay(100);
  }

  Serial.println("EEPROM Cleared");
  

}

void loop() {
  // put your main code here, to run repeatedly:

}
