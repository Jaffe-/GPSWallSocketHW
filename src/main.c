#include <avr/io.h>
#include <stdbool.h>
#include <task/task.h>
#include <usart/usart.h>

void dummy_task(void)
{
    static int cnt = 0;
    if (cnt++ == 30) {
        cnt = 0;
        usart_send_string("hello\n");
    }
}

int main(void)
{
    task_setup();
    usart_setup();
    task_manager();
}
