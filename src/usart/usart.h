#pragma once

void usart_setup();
void usart_send_handler(void);
int8_t usart_send(char *data, uint8_t len);
int8_t usart_send_string(char *str);
