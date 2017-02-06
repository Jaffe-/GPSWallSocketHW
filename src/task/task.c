/*
  Task handler

*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <task/task.h>
#include <usart/usart.h>

struct task tasks[] = {
    { &usart_send_handler, 1, 0}
};

uint8_t num_tasks = sizeof(tasks) / sizeof(struct task);

static volatile uint8_t ticks;

void task_setup()
{
    // Enable interrupts
    sei();

    // CTC operation
    TCCR0A = 0b10 << WGM00;

    // Set period to ~15 kHz
    OCR0A = 4;

    // Enable compare match interrupt
    TIMSK0 = 1 << OCIE0A;

    // Normal operation, clock prescaler 256
    TCCR0B = (1 << FOC0A) | (0b100 << CS00);
}

void task_stop(void)
{
    TIMSK0 = 0;
}

void task_restart(void)
{
    TIMSK0 = 1 << OCIE0A;
}

/*
  Timer interrupt handler
*/
ISR(TIMER0_COMPA_vect)
{
    ticks++;
}

void task_manager(void)
{
    uint8_t last_tick = 0;

    while(1) {
        // Wait until new tick arrives
        while (ticks == last_tick);

        // Loop through each task and update its count
        for (uint8_t i = 0; i < num_tasks; i++)
            tasks[i].counter += ticks - last_tick;

        last_tick = ticks;

        for (uint8_t i = 0; i < num_tasks; i++) {
            // If number of ticks changed after previous task, stop going further
            if (ticks != last_tick)
                break;

            // Call the task/task.handler when the count reaches the period
            if (tasks[i].counter >= tasks[i].period) {
                tasks[i].counter = 0;
                tasks[i].handler();
            }
        }
    }
}
