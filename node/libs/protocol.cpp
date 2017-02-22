#include "protocol.h"

/* Common protocol functions */

void create_msg_on(uint8_t *buffer) {
    buf[0] = ON;
}

void create_msg_off(uint8_t *buffer) {
    buf[0] = OFF;
}

void create_msg_config(uint8_t *buffer, uint32_t address) {
    buf[0] = CONFIG;
    *(static_cast<uint32_t*>(&buffer[1])) = address;
}

void create_msg_init_config(uint8_t *buffer) {
    buf[0] = INIT_CONFIG;
}

void create_msg_status(uint8_t *buffer, RelayState relay_state, ControlState control_state, float current) {
    buf[0] = STATUS;
    buf[1] = relay_state;
    buf[2] = control_state;
    *(static_cast<float*>(&buffer[3])) = current;
}

void decode_msg_config(uint8_t *buffer, uint32_t *address) {
    *address = *(static_cast<uint32_t*>(&buffer[1]));
}

void decode_msg_status(uint8_t *buffer, RelayState *relay_state, ControlState *control_state, float *current) {
    *relay_state = buf[1];
    *control_state = buf[2];
    *current = *(static_cast<float*>(&buffer[3]));
}

bool verify_msg(uint8_t *buffer) {
    MessageType msg_type = get_msg_type(buffer);
    bool found = false;
    for (type : { MessageType::ACK,
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

    if (state == MessageType::STATUS) {
        RelayState rs;
        ControlState cs;
        float current;
        decode_msg_status(buffer, &rs, &cs, &value);

        if (!(rs == RelayState::ON || rs == RelayState::OFF))
            return false;

        if (!(cs == ControlState::ON || cs == ControlState::OFF || cs == ControlState::GEO))
            return false;
    }

    return true;
    }
}
