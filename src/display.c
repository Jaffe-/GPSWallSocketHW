/*
   Display functions

   LCM1602C

   The display is connected as follows:

   Display pin    Signal      Atmega
   4              RS          PB1
   6              E           PB0
   7 - 12         DB0 - DB5   PC0 - PC5
   13 - 14        DB6 - DB7   PB6 - PB7
*/

#include "display.h"
#include <avr/io.h>

#define F_CPU 8000000L

#include <util/delay.h>

// Bitmask for pins on PORTC
#define DB_PORTC_m     0b00111111

// Bitmasks for pins on PORTB
#define DB_PORTB_m     0b11000000
#define RS_PORTB_m     0b00000010
#define E_PORTB_m      0b00000001

// Instructions
#define CLEAR_DISPLAY 1
#define RETURN_HOME 0b10
#define ENTRY_MODE 0b100
#define DISPLAY_CONTROL 0b1000
#define DISPLAY_SHIFT 0b10000
#define FUNCTION_SET 0b100000
#define SET_CGRAM_ADDRESS 0b1000000
#define SET_DDRAM_ADDRESS 0b10000000

// Write modes
#define MODE_INSTRUCTION 0
#define MODE_DATA 1

// Scroll directions
#define RIGHT 0
#define LEFT 1

uint8_t current_screen = 1;

#define nop() asm volatile("nop")

static inline void control(uint8_t D, uint8_t C, uint8_t B);
static inline void entrymode(uint8_t increment, uint8_t shift);
static inline void functionset(uint8_t DL, uint8_t N, uint8_t F);
static inline void shift(uint8_t SC, uint8_t RL);
static inline void cgram_address(uint8_t address);
static inline void ddram_address(uint8_t address);
static void write(uint8_t, uint8_t);

/* low level functions */

static inline void db_set(uint8_t db_value)
{
    // 6 lower bits in PORTC
    PORTC &= ~DB_PORTC_m;
    PORTC |= db_value & DB_PORTC_m;

    // 2 upper bits in PORTB
    PORTB &= ~DB_PORTB_m;
    PORTB |= db_value & DB_PORTB_m;
}

static inline void rs_set(uint8_t rs_value)
{
    PORTB &= ~RS_PORTB_m;
    PORTB |= (rs_value << 1) & RS_PORTB_m;
}

static inline void e_set(uint8_t e_value)
{
    PORTB &= ~E_PORTB_m;
    PORTB |= e_value;
}

static void write(uint8_t mode, uint8_t data)
/*
  mode: instruction or data (0: instruction, 1: data)
  data: value to write
*/
{
    rs_set(mode);
    db_set(data);
    e_set(1);
    nop();
    e_set(0);
    _delay_ms(2);
}

/* Display instructions */

static inline void entrymode(uint8_t increment, uint8_t shift)
// increment - 0: decrements cursor, 1: increments cursor
// shift - 0: no shift, 1: shift
{
    write(MODE_INSTRUCTION, ENTRY_MODE | (increment << 1) | shift);
}

static inline void control(uint8_t D, uint8_t C, uint8_t B)
// D: display on/off
// C: cursor on/off
// B: blink cursor on/off
{
    write(MODE_INSTRUCTION, DISPLAY_CONTROL | (D << 2) | (C << 1) | B);
}

static inline void functionset(uint8_t DL, uint8_t N, uint8_t F)
// DL: data length (0: 4-bit, 1: 8-bit)
// N: number of display lines (0: 1-line, 1: 2-line)
// F: character font (0: 5x8, 1: 5x10) (except when N = 1; then it is 5x8 anyway)
{
    write(MODE_INSTRUCTION, FUNCTION_SET | (DL << 4) | (N << 3) | (F << 2));
}

static inline void shift(uint8_t SC, uint8_t RL)
/*
  SC: shift mode (0: cursor move, 1: display shift)
  RL: direction (0: right, 1: left)
*/
{
    write(MODE_INSTRUCTION, DISPLAY_SHIFT | (SC << 3) | (RL << 2));
}

static inline void cgram_address(uint8_t address)
// address: 6 bit CGRAM address
{
    write(MODE_INSTRUCTION, SET_CGRAM_ADDRESS | (address & 0x3F));
}

static inline void ddram_address(uint8_t address)
// address: 7 bit DDRAM address
{
    write(MODE_INSTRUCTION, SET_DDRAM_ADDRESS | (address & 0x7F));
}


/* External functions */

void display_setup()
{
    // Configure the display pins as output pins
    DDRC |= DB_PORTC_m;
    DDRB |= DB_PORTB_m | RS_PORTB_m | E_PORTB_m;

    // Start reset procedure described in the display controller's datasheet.
    _delay_ms(60);
    functionset(1, 0, 0);
    _delay_ms(10);
    functionset(1, 0, 0);
    _delay_us(200);
    functionset(1, 0, 0);

    // Set 8-bit communication, 2-line display and 5x8 font:
    functionset(1, 1, 1);

    // Turn on and clear display:
    control(1, 0, 0);
    display_clear();

    // Set the cursor to auto increment when writing to DDRAM
    entrymode(1, 0);
}

void display_clear()
{
    write(MODE_INSTRUCTION, CLEAR_DISPLAY);
}

void display_returnhome()
{
    write(MODE_INSTRUCTION, RETURN_HOME);
}


void display_setcursor(uint8_t screen, uint8_t line, uint8_t pos)
/*
  screen: 1, 2
  line: 1, 2
  pos: 1 - 40
*/
{
    uint8_t address = 0;
    if (line == 2)
        address = 0x40;

    if (screen == 2)
        address += 16;

    address += pos - 1;

    ddram_address(address);
}

void display_scroll(uint8_t direction)
{
    shift(1, direction);
}

void display_print_char(char c)
{
    write(MODE_DATA, c);
}

void display_print_string(char* string)
{
    for (uint8_t i = 0; string[i] != 0; i++)
        display_print_char(string[i]);
}

void display_print_integer(int16_t number)
{
    if (number == 0) {
        display_print_char('0');
        return;
    }

    if (number < 0) {
        display_print_char('-');
        number = -number;
    }

    uint8_t digits[6] = {0};

    uint8_t i = 5;
    while (number > 0) {
        uint8_t digit = number % 10;
        digits[i--] = digit;
        number -= digit;
        number /= 10;
    }

    for (uint8_t j = i+1; j < 6; j++) {
        display_print_char('0' + digits[j]);
    }

}

void display_print_float(float f)
{
    if (f < 0) {
        display_print_char('-');
        f = -f;
    }

    uint16_t int_part = (uint16_t)f;

    display_print_integer(int_part);

    display_print_char('.');

    // Subtract the integer part
    f -= int_part;

    // Multiply by 1000 to get last three decimals
    f *= 1000;
    int_part = (int16_t)f;

    // If the last digit is larger than 4, round up
    if (int_part % 10 > 4)
        int_part += 10;

    // Divide by 10 to get two first decimals and print
    display_print_integer(int_part / 10);
}

void display_write_character(uint8_t index, uint8_t pixels[8])
/*
  Writes a new character to the given index (0 - 7) in CGRAM. The 5 least
  significant bits in each row in pixels is the on/off value for each of the 5
  pixels in the corresponding row.
*/
{
    // CGRAM address format is IIIRRR where III contains the character's index
    // while RRR contains the desired row. index is therefore shifted 3 times to
    // the left to form the base of the address:
    uint8_t base_address = index << 3;
    cgram_address(base_address);

    // Write each row successively. After each write the address counter will
    // point to the next byte in CGRAM
    for (uint8_t i = 0; i < 8; i++) {
        write(MODE_DATA, pixels[i]);
    }

    // Write to DDRAM address 0 to avoid accidentally writing to CGRAM
    ddram_address(0);
}


/* Window functions */

#define SCREEN_HEIGHT 4

static uint8_t screens[2][SCREEN_HEIGHT][16];

void screen_setup(void)
{
    for (uint8_t screen = 0; screen < 2; screen++) {
        screen_clear(screen);
    }
}

void screen_print_string(char* string)
{
    for (uint8_t i = 0; string[i] != 0; i++)
        screen_print_char(string[i]);
}

void window_update(uint8_t screen, uint8_t line)
{
    for (uint8_t l = line; l <= line + 1; l++) {
        display_setcursor(screen + 1, l - line + 1, 1);
        for (uint8_t i = 0; i < 16; i++) {
            display_print_char(screens[screen][l][i]);
        }
    }
}

void scroll_window(uint8_t direction)
{
    for (uint8_t i = 0; i < 16; i++) {
        display_scroll(direction);
        _delay_ms(50);
    }
}

void display_move_window(uint8_t screen)
{
    if (current_screen == screen)
        return;

    if (screen == 1)
        scroll_window(LEFT);

    else
        scroll_window(RIGHT);

    current_screen = screen;
}

static uint8_t cur_screen, cur_line, cur_col;

void screen_print_char(char c)
{
    screens[cur_screen][cur_line][cur_col++] = c;
}

void screen_print_integer(int16_t number)
{
    if (number == 0) {
        screen_print_char('0');
        return;
    }

    if (number < 0) {
        screen_print_char('-');
        number = -number;
    }

    uint8_t digits[6] = {0};

    uint8_t i = 5;
    while (number > 0) {
        uint8_t digit = number % 10;
        digits[i--] = digit;
        number -= digit;
        number /= 10;
    }

    for (uint8_t j = i+1; j < 6; j++) {
        screen_print_char('0' + digits[j]);
    }

}

void screen_setcursor(uint8_t screen, uint8_t line, uint8_t col)
{
    cur_screen = screen;
    cur_line = line;
    cur_col = col;
}

void screen_print_float(float f)
{
    if (f < 0) {
        screen_print_char('-');
        f = -f;
    }

    uint16_t int_part = (uint16_t)f;

    screen_print_integer(int_part);

    screen_print_char('.');

    // Subtract the integer part
    f -= int_part;

    // Multiply by 1000 to get last three decimals
    f *= 1000;
    int_part = (int16_t)f;

    // If the last digit is larger than 4, round up
    if (int_part % 10 > 4)
        int_part += 10;

    // Divide by 10 to get two first decimals and print
    screen_print_integer(int_part / 10);
}

void screen_clear(uint8_t screen)
{
    for (uint8_t i = 0; i < SCREEN_HEIGHT; i++) {
        for (uint8_t j = 0; j < 16; j++)
            screens[screen][i][j] = ' ';
    }

}
