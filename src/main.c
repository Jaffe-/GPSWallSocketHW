#include <avr/io.h>
#include "task/task.h"

void blinky() {
    PORTB ^= 1 << 5;
}

int main(void)
{
    task_setup();
    DDRB |= 1 << 5;

    task_manager();
}
