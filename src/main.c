#include <avr/io.h>
#include <stdbool.h>
#include <task/task.h>
#include <usart/usart.h>

int main(void)
{
    task_setup();
    usart_setup();
    usart_send_string(LOG, "Some log line\n");
    usart_send_string(ESP, "AT command\n");
    task_manager();
}
