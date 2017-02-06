#pragma once

#define ESP 0
#define LOG 1

void usart_setup();
void usart_send_handler(void);
int8_t usart_send(uint8_t channel, char *data, uint8_t len);
int8_t usart_send_string(uint8_t channel, char *str);
char usart_read_byte(void);
uint8_t usart_bytes_remaining(void);
void usart_read_remaining(char *dst);
