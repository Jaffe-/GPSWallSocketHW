#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>

#define TX_SIZE 128
#define RX_SIZE 128
#define CHN_SEL_MASK 0b100

static struct {
    uint8_t tx_len[2];
    uint8_t tx_pos[2];
    char tx_buffer[2][TX_SIZE];

    uint8_t rx_read_pos;
    volatile uint8_t rx_write_pos;
    volatile char rx_buffer[RX_SIZE];
} usart_state;

void usart_setup()
{
    // Use divider of 8 for serial clock
    UCSR0A = 1 << U2X0;

    // Enable RX, TX and RX interrupt
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);

    // 8-bit character size
    UCSR0C = 0b011 << UCSZ00;

    // 57.6kbps
    UBRR0L = 34;

    // Use PD2 for channel selection and set it high
    DDRD |= CHN_SEL_MASK;
    PORTD |= CHN_SEL_MASK;
}

int8_t usart_send(uint8_t channel, char *data, uint8_t len)
{
    if (usart_state.tx_len[channel] + len <= TX_SIZE) {
        memcpy(&usart_state.tx_buffer[channel][usart_state.tx_len[channel]], data, len);
        usart_state.tx_len[channel] += len;
        return 0;
    }
    else {
        return -1;
    }
}

int8_t usart_send_string(uint8_t channel, char *str)
{
    return usart_send(channel, str, strlen(str));
}


/*
  This is called regularly by the task manager to send one single byte.

  The ESP ('channel 0') has the highest priority, so if there is a message
  for channel 0, it is handled first.
*/
void usart_send_handler(void)
{
    static uint8_t current_mask = CHN_SEL_MASK;

    for (uint8_t chn = 0; chn < 2; chn++) {
        if (usart_state.tx_len[chn] > 0) {
            // Check whether the TX data buffer is empty
            if (!(UCSR0A & (1 << UDRE0)))
                return;

            // If we needed to switch to channel, wait
            uint8_t mask = (!chn) ? 0 : CHN_SEL_MASK;

            if (current_mask != mask) {
                PORTD &= ~CHN_SEL_MASK;
                PORTD |= mask;
                current_mask = mask;
            }

            // Send next byte and check if we are done
            UDR0 = usart_state.tx_buffer[chn][usart_state.tx_pos[chn]++];
            if (usart_state.tx_pos[chn] == usart_state.tx_len[chn]) {
                usart_state.tx_pos[chn] = 0;
                usart_state.tx_len[chn] = 0;
            }

            break;
        }
    }
}

/* RX interrupt handler */
ISR(USART_RX_vect)
{
    usart_state.rx_buffer[usart_state.rx_write_pos] = UDR0;
    if (++usart_state.rx_write_pos == RX_SIZE)
        usart_state.rx_write_pos = 0;
}

char usart_read_byte(void)
{
    char value = usart_state.rx_buffer[usart_state.rx_read_pos];

    if (++usart_state.rx_read_pos == RX_SIZE)
        usart_state.rx_read_pos = 0;

    return value;
}

uint8_t usart_bytes_remaining(void)
{
    if (usart_state.rx_write_pos >= usart_state.rx_read_pos)
        return usart_state.rx_write_pos - usart_state.rx_read_pos;
    else
        return RX_SIZE - (usart_state.rx_read_pos - usart_state.rx_write_pos);
}

void usart_read_remaining(char *dst)
{
    uint8_t i = 0;
    for (; i < RX_SIZE && usart_bytes_remaining(); i++) {
        dst[i] = usart_read_byte();
    }
    dst[i] = 0;
}
