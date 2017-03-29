#include <string>
#include <iostream>
#include <chrono>
#include <queue>
#include <algorithm>
#include <signal.h>
#include "nrf905.h"
#include "socket.h"
#include "ioexception.h"
#include "json.hpp"
#include "../node/protocol.h"
#include <array>

#define LOG_MODULE "Hub"
#include "log.h"

using json = nlohmann::json;

int detail_enabled;

constexpr uint32_t UNCONFIGURED_ADDRESS = 1;
constexpr int SEND_MAX_TRIES = 5;
constexpr int SUCCESSIVE_SENDS = 5;
constexpr std::chrono::duration<double> DEVICE_UPDATE_TIMEOUT = std::chrono::seconds(1);
constexpr std::chrono::duration<double> SEND_PERIOD = std::chrono::milliseconds(200);
constexpr std::chrono::duration<double> STATISTICS_PERIOD = std::chrono::seconds(1);

json make_event_json(uint32_t address, const std::string& event) {
    return {{"address", address}, {"event", event}};
}

json make_current_json(uint32_t address, float current) {
    return {
        {"address", address},
        {"event", "current_update"},
        {"current", current }
    };
}

class Hub {
private:
    nRF905 nrf;
    Socket socket;
    bool running;

    struct DeviceEntry {
        std::chrono::steady_clock::time_point last_receive_time;
        std::chrono::steady_clock::time_point last_message_sent;
        std::chrono::steady_clock::time_point last_current_update;

        /* Last received updates of these state variables.
           Use these to detect changes that should be reported to the cloud */
        RelayState relay_state;
        ControlState control_state;

        std::array<uint8_t, 32> send_buffer;
        int send_tries;
        enum { FREE, TRANSMITTING, FAILURE } send_state = FREE;
        std::vector<float> current_measurements;
    };

    std::map<uint32_t, DeviceEntry> devices;

    void send(uint32_t address, const std::array<uint8_t, 32>& msg);
    void send_handler();
    void ack_message(uint32_t address);
    void nrf_receive_handler();
    void socket_receive_handler();
    void timeout_handler();
    void current_update_handler();

public:
    Hub();

    void run();
};

Hub::Hub()
    : nrf("/dev/nrf905"),
      socket("/tmp/radio.sock"),
      running(true) {
    nrf.set_rx_address(0);
    nrf.set_listen(true);
    nrf.set_channel(0, 108);  // 108 is 433.200 MHz
    nrf.set_pwr(0);
}

void Hub::ack_message(uint32_t address) {
    assert(devices.find(address) != devices.end());
    devices[address].send_state = DeviceEntry::FREE;
}

void Hub::nrf_receive_handler() {
    std::array<uint8_t, 32> msg_arr = nrf.receive();
    const uint8_t* msg = msg_arr.data();

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

    if (address == UNCONFIGURED_ADDRESS && type != MessageType::INIT_CONFIG) {
        LOG_WARN("Unexpected message from address 1");
        return;
    } else {
        switch (type) {
        case MessageType::ACK:
            ack_message(address);
            return;


        case MessageType::INIT_CONFIG:
            if (address != UNCONFIGURED_ADDRESS) {
                LOG_WARN("INIT_CONFIG should only be sent from address 1");
            } else {
                socket.send(make_event_json(address, "config_request"));
            }
            break;


        case MessageType::STATUS: {
            RelayState relay_state;
            ControlState control_state;
            float currents[5];

            decode_msg_status(msg, relay_state, control_state, currents);

            // TODO: Do something with the current

            if (devices[address].relay_state != relay_state ||
                devices[address].control_state != control_state) {
                if (devices[address].relay_state != relay_state ||
                    control_state == ControlState::GEO) {
                    if (relay_state == RelayState::ON)
                        socket.send(make_event_json(address, "switched_on"));
                    else if (relay_state == RelayState::OFF)
                        socket.send(make_event_json(address, "switched_off"));
                }

                else {
                    if (control_state == ControlState::ON)
                        socket.send(make_event_json(address, "manual_on"));
                    else if (control_state == ControlState::OFF)
                        socket.send(make_event_json(address, "manual_off"));
                }
            }

            devices[address].relay_state = relay_state;
            devices[address].control_state = control_state;
            for (float current : currents) {
                LOG_DETAILED("Current: " << current);
                devices[address].current_measurements.push_back(current);
            }
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
        std::array<uint8_t, 32> buffer;
        create_msg_ack(buffer.data());
        nrf.send(address, buffer);
    }
}

void Hub::socket_receive_handler() {
    std::vector<json> json_packets = socket.receive();
    if (json_packets.empty())
        return;

    for (const auto& json_packet : json_packets) {
        std::array<uint8_t, 32> buffer;

        if (json_packet.find("address") == json_packet.end() ||
            json_packet.find("command") == json_packet.end()) {
            LOG_WARN("Ill-formed JSON received from hub.js");
            continue;
        }

        uint32_t address = json_packet["address"];
        std::string command = json_packet["command"];

        if (command == "on") {
            create_msg_on(buffer.data());
        }
        else if (command == "off") {
            create_msg_off(buffer.data());
        }
        else if (command == "config") {
            uint32_t new_address = json_packet["new_address"];
            create_msg_config(buffer.data(), new_address);
        }
        else {
            LOG_WARN("Unexpected command " << command << " received on socket");
            continue;
        }

        send(address, buffer);
    }
}

void Hub::send(uint32_t address, const std::array<uint8_t, 32>& msg) {
    auto it = devices.find(address);
    if (it == devices.end()) {
        LOG_WARN("Tried to send message to unregistered device.");
        return;
    }

    DeviceEntry& device = it->second;

    LOG("Send is called");
    if (device.send_state != DeviceEntry::FREE) {
        LOG_WARN("NOT FREE DEVICE: " << device.send_state);
    }
    device.send_buffer = msg;
    device.send_tries = 0;
    device.send_state = DeviceEntry::TRANSMITTING;
}

void Hub::send_handler() {
    for (auto& pair : devices) {
        uint32_t address = pair.first;
        DeviceEntry& device = pair.second;

        auto now = std::chrono::steady_clock::now();

        if (device.send_state == DeviceEntry::TRANSMITTING) {
            if (device.send_tries > 0 && device.last_message_sent - now < SEND_PERIOD) {
                continue;
            }
            if (device.send_tries > 0) {
                LOG(LOG_ADDR(address) << "no ACK, retry number " << device.send_tries);
            }
            nrf.send(address, device.send_buffer);
            device.send_tries++;
            device.last_message_sent = now;
            if (device.send_tries == SEND_MAX_TRIES) {
                LOG_WARN(LOG_ADDR(address) << "no ACK, giving up");
                socket.send(make_event_json(address, "offline"));
                device.send_state = DeviceEntry::FAILURE;
            }
        }
    }
}

void Hub::timeout_handler() {
    for (auto it = devices.begin(); it != devices.end();) {
        uint32_t address = it->first;
        DeviceEntry& device = it->second;

        auto now = std::chrono::steady_clock::now();

        if (address != UNCONFIGURED_ADDRESS &&
            now - device.last_receive_time > DEVICE_UPDATE_TIMEOUT) {
            LOG(LOG_ADDR(address) << "timed out (assumed dead)");
            socket.send(make_event_json(address, "offline"));
            it = devices.erase(it);
        }
        else {
            ++it;
        }
    }
}

void Hub::current_update_handler() {
    for (auto& pair : devices) {
        uint32_t address = pair.first;
        auto& device = pair.second;
        if (address == UNCONFIGURED_ADDRESS)
            continue;

        auto now = std::chrono::steady_clock::now();

        if (now - device.last_current_update >= STATISTICS_PERIOD) {
            float average = std::accumulate(device.current_measurements.begin(),
                                            device.current_measurements.end(),
                                            0.0);
            average /= device.current_measurements.size();
            socket.send(make_current_json(address, average * 230));
            device.current_measurements.clear();
            device.last_current_update = now;
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
        else {
            send_handler();
            timeout_handler();
            current_update_handler();
        }
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
