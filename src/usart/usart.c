#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>

#define TX_SIZE 64
#define RX_SIZE 128
#define TX 0b01
#define RX 0b10

struct {
    uint8_t flags;
    uint8_t tx_len;
    uint8_t tx_idx;
    char tx_buffer[TX_SIZE];
    char rx_buffer[RX_SIZE];
} usart_state;

void usart_setup()
{
    // Use divider of 8 for serial clock
    UCSR0A = 1 << U2X0;

    // Enable TX and TX interrupt
    UCSR0B = (1 << TXEN0);

    // 8-bit character size
    UCSR0C = 0b011 << UCSZ00;

    UBRR0L = 16;
}

int8_t usart_send(char *data, uint8_t len)
{
    if (usart_state.tx_len + len <= TX_SIZE) {
        memcpy(usart_state.tx_buffer + usart_state.tx_len, data, len);
        usart_state.tx_len += len;
        usart_state.flags |= TX;
        return 0;
    }
    else {
        return -1;
    }
}

int8_t usart_send_string(char *str)
{
    return usart_send(str, strlen(str));
}

void usart_send_handler(void)
{
    if (usart_state.flags & TX) {
        // Check whether the TX data buffer is empty
        if (!(UCSR0A & (1 << UDRE0)))
            return;

        // Send next byte and check if we are done
        UDR0 = usart_state.tx_buffer[usart_state.tx_idx++];
        if (usart_state.tx_idx == usart_state.tx_len) {
            usart_state.flags &= ~TX;
            usart_state.tx_idx = 0;
            usart_state.tx_len = 0;
        }
    }
}

