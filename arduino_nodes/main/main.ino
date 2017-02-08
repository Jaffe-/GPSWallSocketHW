#include <nRF905_config.h>
#include <nRF905.h>
#include <nRF905_defs.h>
#include <nRF905_types.h>

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
