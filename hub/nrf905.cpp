#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <string>
#include <algorithm>
#include "nrf905.h"
#include "ioexception.h"
#include <array>

#define LOG_MODULE "nRF905"
#include "log.h"

nRF905::nRF905(const std::string& device_file) {
	if ((fd = open(device_file.c_str(), O_RDWR)) == -1) {
        throw IOException("Failed to open nRF905 device", errno);
	}
    LOG("Initialized nRF905 device");
}

nRF905::~nRF905() {
    LOG("Closing device");
    close(fd);
}

void nRF905::set_attribute(const std::string& name, char *data, int size) {
    int fd;
    const std::string filename = "/sys/bus/spi/devices/spi0.0/" + name;

    if ((fd = open(filename.c_str(), O_WRONLY)) == -1) {
        throw IOException("Failed to open sysfs attribute " + filename, errno);
    }

    write(fd, data, size);
    close(fd);
}

void nRF905::set_rx_address(uint32_t address) {
    LOG_DETAILED("Set RX address: " << std::hex << address << std::dec);
    set_attribute("rx_address", reinterpret_cast<char *>(&address), 4);
}

void nRF905::set_tx_address(uint32_t address) {
    LOG_DETAILED("Set TX address: " << std::hex << address << std::dec);
    set_attribute("tx_address", reinterpret_cast<char *>(&address), 4);
}

void nRF905::set_listen(bool listen) {
    LOG("Set listen mode: " << listen);
    char c = listen ? '1' : '0';
    set_attribute("listen", &c, 1);
}

void nRF905::set_frequency(unsigned int freq) {
    assert((freq >= 422400 && freq < 473600) ||
           (freq >= 844800 && freq < 947200));

    LOG("Set frequency: " << freq);
    char buf[10];
    snprintf(buf, sizeof(buf), "%u", freq);
    set_attribute("frequency", buf, sizeof(buf));
}

void nRF905::set_channel(bool hf, int chn) {
    assert(chn >= 0 && chn < 512);

    LOG("Set channel: " << chn << (hf ? " (HF)" : " (LF)"));
    int freq;
    if (!hf)
        freq = 422400 + 100 * chn;
    else
        freq = 844800 + 200 * chn;
    set_frequency(freq);
}

void nRF905::set_pwr(int pwr) {
    assert(pwr >= 0 && pwr <= 3);

    LOG("Set power mode: " << pwr);
    char buf[10];
    snprintf(buf, sizeof(buf), "%hhu", pwr);
    set_attribute("pa_pwr", buf, sizeof(buf));
}

void nRF905::send(uint32_t address, const std::array<uint8_t, 32>& data) {
    set_tx_address(address);
    uint8_t buf[32] {};
    std::copy(data.begin(), data.end(), buf);
    int hex[32] {};
    std::stringstream ss;
    for (int i = 0; i < 32; i++) {
        int byt = buf[i];
        ss << std::hex << byt << " ";
    }
    LOG_DETAILED(LOG_ADDR(address) << " send raw: " << ss.str());
    if (write(fd, buf, 32) == -1) {
        throw IOException("Failed to write to nRF905 device", errno);
    }
}

std::array<uint8_t, 32> nRF905::receive() {
    std::array<uint8_t, 32> buf;

    if (read(fd, reinterpret_cast<char*>(buf.data()), 32) != -1) {
        int hex[32] {};
        std::stringstream ss;
        for (int i = 0; i < 32; i++) {
            int byt = buf[i];
            ss << std::hex << byt << " ";
        }
        LOG_DETAILED("recv raw: " << ss.str());
    }

    return buf;
}
