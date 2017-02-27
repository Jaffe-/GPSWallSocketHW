#include <string>
#include <iostream>
#include <chrono>
#include <queue>
#include <signal.h>
#include "nrf905.h"
#include "socket.h"
#include "ioexception.h"
#include "json.hpp"
#include "../node/libs/protocol.h"

#define LOG_MODULE "Hub"
#include "log.h"

using json = nlohmann::json;

int detail_enabled;

constexpr uint32_t UNCONFIGURED_ADDRESS = 1;
constexpr int SEND_MAX_TRIES = 5;
constexpr int SUCCESSIVE_SENDS = 5;
constexpr std::chrono::duration<double> DEVICE_UPDATE_TIMEOUT = std::chrono::seconds(5);

json make_error_json(uint32_t address, const std::string& error) {
    return {{"address", address}, {"error", error}};
}

json make_event_json(uint32_t address, const std::string& event) {
    return {{"address", address}, {"event", event}};
}

class Hub {
private:
    nRF905 nrf;
    Socket socket;
    bool running;

    struct DeviceEntry {
        std::chrono::steady_clock::time_point last_receive_time;
        std::chrono::steady_clock::time_point last_message_sent;

        /* Last received updates of these state variables.
           Use these to detect changes that should be reported to the cloud */
        RelayState relay_state;
        ControlState control_state;

        std::basic_string<uint8_t> send_buffer{32,0};
        int send_tries;
        enum { FREE, TRANSMITTING, FAILURE } send_state;
    };

    std::map<uint32_t, DeviceEntry> devices;

    void send(uint32_t address, const std::basic_string<uint8_t>& msg);
    void send_handler();
    void ack_message(uint32_t address);
    void nrf_receive_handler();
    void socket_receive_handler();
    void timeout_handler();

public:
    Hub();

    void run();
};

Hub::Hub()
    : nrf("/dev/nrf905"),
      socket("/home/pi/radio.sock"),
      running(true) {
    nrf.set_rx_address(0x586F2E10);
    nrf.set_tx_address(0xFE4CA6E5);
    nrf.set_listen(true);
    nrf.set_channel(0, 108);  // 108 is 433.200 MHz
    nrf.set_pwr(0);
}

void Hub::ack_message(uint32_t address) {
    assert(devices.find(address) != devices.end());
    devices[address].send_state = DeviceEntry::FREE;
}

void Hub::nrf_receive_handler() {
    std::basic_string<uint8_t> s_msg = nrf.receive();
    const uint8_t *msg = s_msg.data();

    uint32_t address = get_msg_address(msg);
    MessageType type = get_msg_type(msg);

    if (address != UNCONFIGURED_ADDRESS && devices.find(address) == devices.end()) {
        devices[address] = DeviceEntry();
        socket.send(make_event_json(address, "device_online"));
    }
    devices[address].last_receive_time = std::chrono::steady_clock::now();

    if (!verify_msg(msg)) {
        LOG_WARN(LOG_ADDR(address) << "received ill-formed message");
        return;
    }
    LOG_DETAILED(LOG_ADDR(address) << "received " << get_msg_type_string(type));

    switch (type) {
    case MessageType::ACK:
        ack_message(address);
        return;


        /* Messages that are forwarded straight to the cloud */

    case MessageType::INIT_CONFIG:
        if (address != UNCONFIGURED_ADDRESS) {
            LOG_WARN("INIT_CONFIG should only be sent from address 1");
        } else {
            socket.send(make_event_json(address, "config_request"));
        }
        break;


        /* Messages that we should handle here */

    case MessageType::STATUS: {
        RelayState relay_state;
        ControlState control_state;
        float current;

        decode_msg_status(msg, &relay_state, &control_state, &current);

        // TODO: Do something with the current

        /* Tell the cloud if the relay state changed for some reason */
        if (devices[address].relay_state != relay_state) {
            if (relay_state == RelayState::ON)
                socket.send(make_event_json(address, "switched_on"));
            else if (relay_state == RelayState::OFF)
                socket.send(make_event_json(address, "switched_off"));
        }

        if (devices[address].control_state != control_state) {
            if (control_state == ControlState::ON ||
                control_state == ControlState::OFF) {
                socket.send(make_event_json(address, "control_manual"));
            }
            else if (control_state == ControlState::GEO)
                socket.send(make_event_json(address, "control_geo"));
        }

        devices[address].relay_state = relay_state;
        devices[address].control_state = control_state;
        break;
    }


        /* Messages that should not be received by the hub */

    case MessageType::ON:
    case MessageType::OFF:
    case MessageType::CONFIG:
    default:
        LOG_WARN("Unexpected message to be received from node");
        break;
    }

    /* Acknowledge the message regardless */
    uint8_t buffer[32] {};
    create_msg_ack(buffer);
    nrf.send(address, std::basic_string<uint8_t>{buffer, 32});
}

void Hub::socket_receive_handler() {
    std::vector<json> json_packets = socket.receive();
    if (json_packets.empty())
        return;

    for (const auto& json_packet : json_packets) {
        uint8_t buffer[32] {};

        uint32_t address = json_packet["address"];
        std::string command = json_packet["command"];

        if (command == "on") {
            create_msg_on(buffer);
        }
        else if (command == "off") {
            create_msg_off(buffer);
        }
        else if (command == "config") {
            uint32_t new_address = json_packet["new_address"];
            create_msg_config(buffer, new_address);
        }
        else {
            LOG_WARN("Unexpected command " << command << " received on socket");
            continue;
        }

        send(address, std::basic_string<uint8_t>{buffer, 32});
    }
}

void Hub::send(uint32_t address, const std::basic_string<uint8_t>& msg) {
    auto it = devices.find(address);
    if (it == devices.end()) {
        LOG_WARN("Tried to send message to unregistered device.");
        socket.send(make_error_json(address, "device_offline"));
        return;
    }

    DeviceEntry& device = it->second;

    if (device.send_state == DeviceEntry::FREE) {
        device.send_buffer = msg;
        device.send_tries = 0;
        device.send_state = DeviceEntry::TRANSMITTING;
    }
    else {
        socket.send(make_error_json(address, "transmit_in_progress"));
    }
}

void Hub::send_handler() {
    for (auto& pair : devices) {
        uint32_t address = pair.first;
        DeviceEntry& device = pair.second;

        if (device.send_state == DeviceEntry::TRANSMITTING) {
            if (device.send_tries > 0) {
                LOG(LOG_ADDR(address) << "no ACK, retry number " << device.send_tries);
            }
            nrf.send(address, device.send_buffer);
            device.send_tries++;
            if (device.send_tries == SEND_MAX_TRIES) {
                LOG_WARN(LOG_ADDR(address) << "no ACK, giving up");
                socket.send(make_error_json(address, "device_failure"));
                device.send_state = DeviceEntry::FAILURE;
            }
        }
    }
}

void Hub::timeout_handler() {
    for (auto it = devices.begin(); it != devices.end();) {
        uint32_t address = it->first;
        DeviceEntry& device = it->second;

        if (std::chrono::steady_clock::now() - device.last_receive_time > DEVICE_UPDATE_TIMEOUT) {
            LOG(LOG_ADDR(address) << "timed out (assumed dead)");
            socket.send(make_event_json(address, "offline"));
            it = devices.erase(it);
        }
        else {
            ++it;
        }
    }
}

void Hub::run() {
    while (running) {
        fd_set readfds;
        int nrf_fd = nrf.get_fd();
        int socket_fd = socket.get_fd();

        FD_ZERO(&readfds);
        FD_SET(nrf.get_fd(), &readfds);
        FD_SET(socket.get_fd(), &readfds);
        int max_fd = nrf_fd > socket_fd ? nrf_fd : socket_fd;

        struct timeval tv;
        tv.tv_usec = 100000;
        tv.tv_sec = 0;
        if (select(max_fd + 1, &readfds, NULL, NULL, &tv) == -1) {
            throw IOException("select() failed", errno);
        }

        if (FD_ISSET(nrf_fd, &readfds)) {
            nrf_receive_handler();
        }
        else if (FD_ISSET(socket_fd, &readfds)) {
            socket_receive_handler();
        }

        send_handler();
        timeout_handler();
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
        Hub hub;

        hub.run();
    }
    catch (IOException& e) {
        std::cout << "IO error: " << e.what << std::endl;
    }
}
