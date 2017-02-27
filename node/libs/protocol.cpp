#include "protocol.h"
#include <initializer_list>
#include <string.h>

/* Common protocol functions */

#define FIELDS_START 5    // 1 byte type + 4 byte address

static uint32_t my_address;

void protocol_set_my_address(uint32_t address) {
    my_address = address;
}


void serialize(uint8_t *buffer) {}

template <typename T, typename... Rest>
void serialize(uint8_t *buffer, T first, Rest... rest) {
    *(reinterpret_cast<T*>(buffer)) = first;
    serialize(buffer + sizeof(T), rest...);
}

void create_msg_ack(uint8_t *buffer) {
    serialize(buffer, MessageType::ACK, my_address);
}

void create_msg_on(uint8_t *buffer) {
    serialize(buffer, MessageType::ON, my_address);
}

void create_msg_off(uint8_t *buffer) {
    serialize(buffer, MessageType::OFF, my_address);
}

void create_msg_config(uint8_t *buffer, uint32_t new_address) {
    serialize(buffer, MessageType::CONFIG, my_address, new_address);
}

void create_msg_init_config(uint8_t *buffer) {
    serialize(buffer, MessageType::INIT_CONFIG, my_address);
}

void create_msg_status(uint8_t *buffer, RelayState relay_state, ControlState control_state, float current) {
    serialize(buffer, MessageType::STATUS, my_address, relay_state, control_state, current);
}


/* Decode functions */

MessageType get_msg_type(const uint8_t *buffer) {
    return *(reinterpret_cast<const MessageType*>(&buffer[0]));
}

uint32_t get_msg_address(const uint8_t *buffer) {
    uint32_t addr;
    memcpy(reinterpret_cast<uint8_t*>(&addr), &buffer[1] , sizeof(uint32_t));
    return addr;
}

void deserialize(const uint8_t *buffer) {}

template <typename T, typename... Rest>
void deserialize(const uint8_t *buffer, T* first, Rest*... rest) {
    memcpy(reinterpret_cast<uint8_t*>(first), buffer + FIELDS_START, sizeof(T));
    deserialize(buffer + sizeof(T), rest...);
}

void decode_msg_config(const uint8_t *buffer, uint32_t *new_address) {
    deserialize(buffer, new_address);
}

void decode_msg_status(const uint8_t *buffer, RelayState *relay_state, ControlState *control_state, float *current) {
    deserialize(buffer, relay_state, control_state, current);
}


/* Verification that message is OK */

bool verify_msg(const uint8_t *buffer) {
    MessageType msg_type = get_msg_type(buffer);
    bool found = false;
    for (auto type : { MessageType::ACK,
                       MessageType::ON,
                       MessageType::OFF,
                       MessageType::CONFIG,
                       MessageType::INIT_CONFIG,
                       MessageType::STATUS }) {
        if (type == msg_type) {
            found = true;
            break;
        }
    }

    if (!found)
        return false;

    if (msg_type == MessageType::STATUS) {
        RelayState rs;
        ControlState cs;
        float current;
        decode_msg_status(buffer, &rs, &cs, &current);

        if (!(rs == RelayState::ON || rs == RelayState::OFF))
            return false;

        if (!(cs == ControlState::ON || cs == ControlState::OFF || cs == ControlState::GEO))
            return false;
    }

    return true;
}
