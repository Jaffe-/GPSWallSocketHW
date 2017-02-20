#include "libs/nRF905_config.h"
#include "libs/nRF905.h"
#include "libs/nRF905_defs.h"
#include "libs/nRF905_types.h"
#include "types.h"
#include "libs/protocol.h"

void setup()
{
    Serial.begin(9600);
    radio_setup();
    LED_setup();
    relay_setup();
    switches_setup();
}

void loop()
{
    Serial.println("I'm alive?");
    delay(500);
    LED_on();
    delay(500);
    LED_off();
}
