#include <string>
#include <iostream>
#include <chrono>
#include <queue>
#include <signal.h>
#include "nrf905.h"
#include "socket.h"
#include "ioexception.h"
#include "poller.h"
#include "json.hpp"
#include "../node/libs/protocol.h"

#define LOG_MODULE "Hub"
#include "log.h"

using json = nlohmann::json;

int detail_enabled;

constexpr uint32_t UNCONFIGURED_ADDRESS = 1;
constexpr int SEND_MAX_TRIES = 5;
constexpr std::chrono::duration<double> DEVICE_UPDATE_TIMEOUT = std::chrono::seconds(5);

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

json enclose_json(const std::string& kind, uint32_t address, const json &data) {
    json js;
    js["address"] = address;
    js[kind] = data;
    return js;
}

json make_message_json(uint32_t address, MessageType type) {
    json msg_js;
    msg_js["type"] = message_type_str(type);
    return enclose_json("message", address, msg_js);
}

json make_error_json(uint32_t address, const std::string& error) {
    return enclose_json("error", address, error);
}

json make_event_json(uint32_t address, const std::string& event) {
    return enclose_json("event", address, event);
}

class Hub {
private:
    Poller poller;
    nRF905 nrf;
    Socket socket;
    bool running;

    std::queue<uint32_t> send_queue;

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
    : nrf("/dev/nrf905", poller, [this] () { nrf_receive_handler(); }),
      socket("/var/run/radio.sock", poller, [this] () { socket_receive_handler(); }),
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
    const uint8_t *msg = s_msg.c_str();

    uint32_t address = get_msg_address(msg);
    MessageType type = get_msg_type(msg);

    if (devices.find(address) == devices.end()) {
        devices[address] = DeviceEntry();
    }
    devices[address].last_receive_time = std::chrono::steady_clock::now();

    if (!verify_msg(msg)) {
        LOG("Ill-formed message received from " << std::hex << address << std::dec);
        return;
    }

    switch (type) {

        /* Messages that are forwarded straight to the cloud */
    case MessageType::INIT_CONFIG:
        LOG("Received INIT_CONFIG from " << std::hex << address << std::dec);
        if (address != UNCONFIGURED_ADDRESS) {
            LOG("INIT_CONFIG should only be sent from address 1");
        } else {
            socket.send(make_message_json(address, MessageType::INIT_CONFIG).dump());
        }
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

        /* Tell the cloud if the relay state changed for some reason */
        if (devices[address].relay_state != relay_state) {
            if (relay_state == RelayState::ON)
                socket.send(make_message_json(address, MessageType::ON).dump());
            else
                socket.send(make_message_json(address, MessageType::OFF).dump());
        }
        // TODO: what do we do when ControlState changes?
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

void Hub::socket_receive_handler() {
    std::string msg = socket.receive();
    if (msg.empty())
        return;

    uint8_t buffer[32];

    LOG("recv: " << msg);
    json recv_json (json::parse(msg));
    uint32_t address = recv_json["address"];
    json msg_json (recv_json["message"]);
    std::string type = msg_json["type"];

    if (type == "on") {
        create_msg_on(buffer);
    }
    else if (type == "off") {
        create_msg_off(buffer);
    }
    else if (type == "config") {
        uint32_t new_address = msg_json["new_address"];
        create_msg_config(buffer, new_address);
    }
    else {
        LOG("Unexpected message " << type << " received on socket");
        return;
    }

    send(address, buffer);
}

void Hub::send(uint32_t address, const std::basic_string<uint8_t>& msg) {
    auto it = devices.find(address);
    if (it == devices.end()) {
        LOG("Tried to send message to unregistered device.");
        socket.send(make_error_json(address, "device_offline").dump());
        return;
    }

    DeviceEntry& device = it->second;

    if (device.send_state == DeviceEntry::FREE) {
        device.send_buffer = msg;
        device.send_tries = 0;
        device.send_state = DeviceEntry::TRANSMITTING;
        send_queue.push(address);
    }
    else {
        socket.send(make_error_json(address, "transmit_in_progress").dump());
    }
}

void Hub::send_handler() {
    while (!send_queue.empty()) {
        uint32_t address = send_queue.front();
        DeviceEntry& device = devices[address];

        if (device.send_state == DeviceEntry::TRANSMITTING) {
            nrf.send(address, device.send_buffer);
            device.send_tries++;
            if (device.send_tries == SEND_MAX_TRIES) {
                socket.send(make_error_json(address, "device_failure").dump());
                device.send_state = DeviceEntry::FAILURE;
            }
        }

        send_queue.pop();
    }
}

void Hub::timeout_handler() {
    for (auto it = devices.begin(); it != devices.end();) {
        uint32_t address = it->first;
        DeviceEntry& device = it->second;

        if (std::chrono::steady_clock::now() - device.last_receive_time > DEVICE_UPDATE_TIMEOUT) {
            LOG("Device at address " << std::hex << address << std::dec << " timed out (assumed dead)");
            socket.send(make_event_json(address, "offline").dump());
            it = devices.erase(it);
        }
        else {
            ++it;
        }
    }
}

void Hub::run() {
    while (running) {
        poller.run();
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
