#include <string>
#include <iostream>
#include <chrono>
#include <signal.h>
#include "nrf905.h"
#include "socket.h"
#include "ioexception.h"
#include "poller.h"
#include "json.hpp"
#include "../node/libs/protocol.h"

#define LOG_MODULE "Radio"
#include "log.h"

using json = nlohmann::json;

int detail_enabled;

class Radio {
private:
    Poller poller;
    nRF905 nrf;
    Socket socket;
    bool running;

    struct DeviceEntry {
        std::chrono::steady_clock::time_point last_state_update;
        std::chrono::steady_clock::time_point last_message_sent;

        /* Last received updates of these state variables.
           Use these to detect changes that should be reported to the cloud */
        RelayState relay_state;
        ControlState control_state;

        uint8_t send_buffer[32];
        bool send_pending;
    };

    std::map<uint32_t, DeviceEntry> devices;

    void ack_message(uint32_t address);
    void nrf_receive_handler();
    void socket_receive_handler();

public:
    Radio();

    void run();
};

Radio::Radio()
    : nrf("/dev/nrf905", poller, [this] () { nrf_receive_handler(); }),
      socket("/var/run/radio.sock", poller, [this] () { socket_receive_handler(); }),
      running(true) {
    nrf.set_rx_address(0x586F2E10);
    nrf.set_tx_address(0xFE4CA6E5);
    nrf.set_listen(true);
    nrf.set_channel(0, 108);  // 108 is 433.200 MHz
    nrf.set_pwr(0);
}

void Radio::ack_message(uint32_t address) {
    assert(devices.find(address) != devices.end());
    devices[address].send_pending = false;
}

std::string message_type_str(MessageType type) {
    switch (type) {
    case MessageType::ACK:
        return "ack";
    case MessageType::ON:
        return "on";
    case MessageType::OFF:
        return "off";
    case MessageType::CONFIG:
        return "config";
    case MessageType::INIT_CONFIG:
        return "init_config";
    case MessageType::STATUS:
        return "status";
    default:
        return "invalid";
    }
}

json make_json(MessageType type, uint32_t address) {
    json js;
    js["message"] = message_type_str(type);
    js["address"] = address;
    return js;
}

void Radio::nrf_receive_handler() {
    std::basic_string<uint8_t> s_msg = nrf.receive();
    const uint8_t *msg = s_msg.c_str();

    uint32_t address = get_msg_address(msg);
    MessageType type = get_msg_type(msg);
    std::chrono::steady_clock::time_point receive_time = std::chrono::steady_clock::now();

    if (!verify_msg(msg)) {
        LOG("Ill-formed message received from " << std::hex << address << std::dec);
        return;
    }

    switch (type) {

        /* Messages that are forwarded straight to the cloud */
    case MessageType::INIT_CONFIG:
        socket.send(make_json(type, address).dump());
        break;

        /* Messages that we should handle here */
    case MessageType::ACK:
        ack_message(address);
        break;

    case MessageType::STATUS: {
        RelayState relay_state;
        ControlState control_state;
        float current;

        decode_msg_status(msg, &relay_state, &control_state, &current);

        // TODO: Do something with the current

        if (devices.find(address) != devices.end()) {

            /* Tell the cloud if the relay state changed for some reason */
            if (devices[address].relay_state != relay_state) {
                json msg_json;
                if (relay_state == RelayState::ON)
                    msg_json = make_json(MessageType::ON, address);
                else
                    msg_json = make_json(MessageType::OFF, address);
                socket.send(msg_json.dump());
            }
            // TODO: what do we do when ControlState changes?
        }
        else {
            devices[address] = DeviceEntry();
        }
        devices[address].last_state_update = receive_time;
        devices[address].relay_state = relay_state;
        devices[address].control_state = control_state;
        break;
    }

        /* Messages that should not be received by the hub */
    case MessageType::ON:
    case MessageType::OFF:
    case MessageType::CONFIG:
    default:
        LOG("Unexpected message " << message_type_str(type) << " received from " << std::hex << address << std::dec);
        break;
    }
}

void Radio::socket_receive_handler() {
    std::string msg = socket.receive();
    if (msg.empty())
        return;

    
}

void Radio::run() {
    while (running) {
        poller.run();
    }
}

int main(int argc, char **argv)
{
    // SIGPIPE happens if the client disconnects; ignore it to avoid
    // crashing when this happens.
    signal(SIGPIPE, SIG_IGN);

    if (argc == 1) {
        detail_enabled = 0;
    }
    else if (argc == 2 && std::string(argv[1]) == "-detailed") {
        detail_enabled = 1;
    }
    else {
        std::cout << "usage: radio [-detailed]" << std::endl;
        return -1;
    }

    try {
        Radio radio;

        radio.run();
    }
    catch (IOException& e) {
        std::cout << "IO error: " << e.what << std::endl;
    }
}
