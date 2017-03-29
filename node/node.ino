#include "libs/nRF905_config.h"
#include "libs/nRF905.h"
#include "libs/nRF905_defs.h"
#include "libs/nRF905_types.h"
#include "types.h"
#include "pinout.h"
#include "protocol.h"
#include "TimerOne.h"

void setup()
{
    Serial.begin(9600);
    LED_setup();
    ConfigureButton_setup();
    relay_setup();
    switches_setup();
    radio_setup();
    currentSensor_setup();
}

void loop()
{
    //Serial.println("I'm alive?");
//    LED_test();
    //Serial.println(analogRead(0));
    //delay(5);
    Cofigure_loop();
    radio_loop();
    control_relay();
}
