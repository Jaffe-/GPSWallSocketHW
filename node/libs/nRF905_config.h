/*
 * Project: nRF905 AVR/Arduino Library/Driver
 * Author: Zak Kemble, contact@zakkemble.co.uk
 * Copyright: (C) 2013 by Zak Kemble
 * License: GNU GPL v3 (see License.txt)
 * Web: http://blog.zakkemble.co.uk/nrf905-avrarduino-librarydriver/
 */

#ifndef NRF905_CONFIG_H_
#define NRF905_CONFIG_H_

#include "nRF905_defs.h"

// Crystal frequency (the one the radio IC/module is using)
// NRF905_CLK_4MHZ
// NRF905_CLK_8MHZ
// NRF905_CLK_12MHZ
// NRF905_CLK_16MHZ
// NRF905_CLK_20MHZ
#define NRF905_CLK_FREQ		NRF905_CLK_16MHZ

// Use pin interrupt for data ready
// NOTE: If you have other devices connected that use the SPI bus then you will need to call nRF905_interrupt_off() before using SPI comms and then RF905_interrupt_on() once you've finished.
#define NRF905_INTERRUPTS	1

// Buffer count
// Only used if interrupts are used (see NRF905_INTERRUPTS)
// NOT YET IMPLEMENTED
//#define BUFFER_COUNT_RX	1

// Buffer count
// NOT YET IMPLEMENTED
//#define BUFFER_COUNT_TX	1

//
// NOT YET IMPLEMENTED
//#define NRF905_MAX_PACKET_SIZE 64

// Use software to get address match state instead of reading pin for high/low state
// Not used in this library yet
#define NRF905_AM_SW		0

// Use software to get data ready state instead of reading pin for high/low state
// Interrupts and software DR can not be enabled together
#define NRF905_DR_SW		0

// Don't transmit if airway is busy (other transmissions are going on)
#define NRF905_COLLISION_AVOID	1


///////////////////
// Pin stuff
///////////////////

#ifdef ARDUINO

// Arduino pins
#define TRX_EN		7	// Enable/standby pin
#define PWR_MODE	8	// Power mode pin
#define TX_EN		9	// TX / RX mode pin
#define CD			2	// Carrier detect pin (for collision avoidance, if enabled)
#define CSN			10	// SPI slave select pin

// Data ready pin
// If using interrupts (NRF905_INTERRUPTS 1) then this must be
// an external interrupt pin that matches the interrupt register settings below.
#define DR			3

// Address match pin (not used by library)
// blah
//#define AM			4

#else
// Non-Arduino pins

// Enable/standby pin
#define CFG_TRX_EN_PORT		D
#define CFG_TRX_EN_BIT		7

// Power mode pin
#define CFG_PWR_MODE_PORT	B
#define CFG_PWR_MODE_BIT	0

// TX / RX mode pin
#define CFG_TX_EN_PORT		B
#define CFG_TX_EN_BIT		1

// Carrier detect pin (for collision avoidance, if enabled)
#define CFG_CD_PORT			D
#define CFG_CD_BIT			2

// Address match pin (not used by library)
// blah
//#define CFG_AM_PORT			D
//#define CFG_AM_BIT			4

// Data ready pin
// If using interrupts (NRF905_INTERRUPTS 1) then this must be
// an external interrupt pin that matches the interrupt register settings below.
#define CFG_DR_PORT			D
#define CFG_DR_BIT			3

// SPI slave select pin
#define CFG_CSN_PORT		B
#define CFG_CSN_BIT			2

#endif


///////////////////
// Interrupt register stuff
// Only needed if NRF905_INTERRUPTS is 1
///////////////////

// Interrupt number (INT0, INT1 etc)
// This must match the INT that is connected to DR
#define INTERRUPT_NUM	1

// ATmega48/88/168/328
// INT0 = D2 (Arduino UNO pin 2)
// INT1 = D3 (Arduino UNO pin 3)

// ATmega640/1280/1281/2560/2561
// INT0 = D0 (Arduino MEGA pin 21)
// INT1 = D1 (Arduino MEGA pin  20)
// INT2 = D2 (Arduino MEGA pin 19)
// INT3 = D3 (Arduino MEGA pin 18)
// INT4 = E4 (Arduino MEGA pin 2)
// INT5 = E5 (Arduino MEGA pin 3)
// INT6 = E6 (Arduino MEGA N/A)
// INT7 = E7 (Arduino MEGA N/A)

// Leave these commented out to let the library figure out what registers to use

// Which interrupt to use for data ready (DR)
//#define REG_EXTERNAL_INT	EIMSK
//#define BIT_EXTERNAL_INT	INT1
//#define INT_VECTOR			INT1_vect

// Set interrupt to trigger on rising edge
//#define REG_EXTERNAL_INT_CTL	EICRA
//#define BIT_EXTERNAL_INT_CTL	(_BV(ISC11)|_BV(ISC10))


///////////////////
// Default radio settings
///////////////////

// Frequency
#define NRF905_FREQ			433200000UL

// Frequency band
// NRF905_BAND_433
// NRF905_BAND_868
// NRF905_BAND_915
#define NRF905_BAND			NRF905_BAND_433

// Output power
// n means negative, n10 = -10
// NRF905_PWR_n10 (-10dBm = 100uW)
// NRF905_PWR_n2 (-2dBm = 631uW)
// NRF905_PWR_6 (6dBm = 4mW)
// NRF905_PWR_10 (10dBm = 10mW)
#define NRF905_PWR			NRF905_PWR_10

// Save a few mA by reducing receive sensitivity
// NRF905_LOW_RX_DISABLE
// NRF905_LOW_RX_ENABLE
#define NRF905_LOW_RX		NRF905_LOW_RX_DISABLE

// Constantly retransmit payload while in transmit mode
// Can be useful in areas with lots of interference, but you'll need to make sure you can differentiate between re-transmitted packets and new packets (like an ID number).
// It will also block other transmissions if collision avoidance is enabled.
// NRF905_AUTO_RETRAN_DISABLE
// NRF905_AUTO_RETRAN_ENABLE
#define NRF905_AUTO_RETRAN	NRF905_AUTO_RETRAN_DISABLE

// Output a clock signal on pin 3 of IC
// NRF905_OUTCLK_DISABLE
// NRF905_OUTCLK_500KHZ
// NRF905_OUTCLK_1MHZ
// NRF905_OUTCLK_2MHZ
// NRF905_OUTCLK_4MHZ
#define NRF905_OUTCLK		NRF905_OUTCLK_DISABLE

// CRC checksum
// NRF905_CRC_DISABLE
// NRF905_CRC_8
// NRF905_CRC_16
#define NRF905_CRC			NRF905_CRC_16

// Address size
// Number of bytes for address
// NRF905_ADDR_SIZE_1
// NRF905_ADDR_SIZE_4
#define NRF905_ADDR_SIZE	NRF905_ADDR_SIZE_4

// Payload size (1 - 32)
#define NRF905_PAYLOAD_SIZE	32 //NRF905_MAX_PAYLOAD

#endif /* NRF905_CONFIG_H_ */