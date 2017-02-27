#pragma once

#include <stdint.h>

class nRF905 {
private:
    int fd;

    int open_attribute(const std::string& name);
    void set_attribute(const std::string& name, char *data, int size);
    void get_attribute(const std::string& name, char *data, int size);

public:
    nRF905(const std::string& device_file);
    ~nRF905();

    int get_fd() const { return fd; }
    void set_tx_address(uint32_t address);
    void set_rx_address(uint32_t address);
    void set_listen(bool listen);
    void set_frequency(unsigned int freq);
    void set_pwr(int pwr);
    void set_channel(bool hf, int chn);
    void send(uint32_t address, const std::basic_string<uint8_t>& data);
    std::basic_string<uint8_t> receive();
};
