#include <string>
#include <iostream>
#include <signal.h>
#include "nrf905.h"
#include "socket.h"
#include "ioexception.h"
#include "poller.h"
#include "rapidjson/document.h"
#include "../node/libs/protocol.h"

#define LOG_MODULE "Radio"
#include "log.h"

using namespace rapidjson;

int detail_enabled;

class Radio {
private:
    Poller poller;
    nRF905 nrf;
    Socket socket;
    bool running;

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

void Radio::nrf_receive_handler() {
    std::string msg = nrf.receive();
    if (socket.connected())
        socket.send(msg);
}

void Radio::socket_receive_handler() {
    std::string msg = socket.receive();
    if (!msg.empty()) {
        Document json;
        json.Parse(msg.c_str());
        Value& command = json["command"];
        std::string cmd = command.GetString();
        LOG("Command: " << cmd);
        if (cmd == "close")
            running = false;
        else if (cmd == "send") {
            std::string message = json["data"].GetString();
            LOG("Sending: " << message.c_str());
            nrf.send(message);
        }
    }
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
