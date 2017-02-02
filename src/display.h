#pragma once
#include <avr/io.h>

void display_setup();
void display_clear();
void display_returnhome();
void display_setcursor(uint8_t screen, uint8_t line, uint8_t pos);
void display_scroll(uint8_t direction);
void display_move_window(uint8_t screen);
void display_write_character(uint8_t index, uint8_t pixels[8]);

void display_print_string(char* string);
void display_print_integer(int16_t number);
void display_print_char(char c);
void display_print_float(float f);

void window_update(uint8_t screen, uint8_t line);
void screen_print_string(char* string);
void screen_print_integer(int16_t number);
void screen_print_float(float f);
void screen_print_char(char c);
void screen_setcursor(uint8_t screen, uint8_t line, uint8_t col);
void screen_setup(void);
void screen_clear(uint8_t screen);
