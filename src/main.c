#include <avr/io.h>
#include "display.h"

int main(void)
{
    display_setup();
    display_clear();
    display_print_string("Hei");

    while(1);
}
