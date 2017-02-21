#pragma once

#include <stdint.h>
#include "poller.h"

class nRF905 {
private:
    int fd;

    int open_attribute(const std::string& name);
    void set_attribute(const std::string& name, char *data, int size);
    void get_attribute(const std::string& name, char *data, int size);

public:
    nRF905(const std::string& device_file, Poller& poller, const std::function<void(void)>& recv_handler);
    ~nRF905();

    int get_fd() const { return fd; }
    void set_tx_address(uint32_t address);
    void set_rx_address(uint32_t address);
    void set_listen(bool listen);
    void set_frequency(unsigned int freq);
    void set_pwr(int pwr);
    void set_channel(bool hf, int chn);
    void send(const std::string& data);
    std::string receive();
};
