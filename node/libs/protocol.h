#pragma once
#include <cstdint>

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

MessageType get_msg_type(uint8_t *buffer);

void create_msg_on(uint8_t *buffer);
void create_msg_off(uint8_t *buffer);
void create_msg_config(uint8_t *buffer, uint32_t address);
void create_msg_init_config(uint8_t *buffer);
void create_msg_status(uint8_t *buffer, RelayState relay_state, ControlState control_state, float current);

void decode_msg_config(uint8_t *buffer, uint32_t *address);
void decode_msg_status(uint8_t *buffer, RelayState *relay_state, ControlState *control_state, float *current);

bool verify_msg(uint8_t *buffer);
