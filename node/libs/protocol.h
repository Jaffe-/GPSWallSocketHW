#pragma once
#include <stdint.h>

/* Header file for common protocol functions */

enum class MessageType : uint8_t {
    ACK = 0xFF,
    ON = 0xF0,
    OFF = 0x0F,
    CONFIG = 0xA0,
    INIT_CONFIG = 0x0A,
    STATUS = 0x55,
};

enum class RelayState : uint8_t {
    ON = 0xF0, OFF = 0x0F
};

enum class ControlState : uint8_t {
    ON = 0xF0, OFF = 0x0F, GEO = 0xFF
};

void protocol_set_my_address(uint32_t address);

void create_msg_ack(uint8_t *buffer);
void create_msg_on(uint8_t *buffer);
void create_msg_off(uint8_t *buffer);
void create_msg_config(uint8_t *buffer, uint32_t address);
void create_msg_init_config(uint8_t *buffer);
void create_msg_status(uint8_t *buffer, RelayState relay_state, ControlState control_state, float current);

MessageType get_msg_type(const uint8_t *buffer);
uint32_t get_msg_address(const uint8_t *buffer);
void decode_msg_config(const uint8_t *buffer, uint32_t *address);
void decode_msg_status(const uint8_t *buffer, RelayState *relay_state, ControlState *control_state, float *current);

bool verify_msg(const uint8_t *buffer);
