#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <string>
#include "nrf905.h"
#include "ioexception.h"

#define LOG_MODULE "nRF905"
#include "log.h"

nRF905::nRF905(const std::string& device_file, Poller &poller, const std::function<void(void)>& recv_handler) {
	if ((fd = open(device_file.c_str(), O_RDWR)) == -1) {
        throw IOException("Failed to open nRF905 device", errno);
	}
    poller.add_handler(fd, recv_handler);
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
    LOG("Set RX address: " << std::hex << address << std::dec);
    set_attribute("rx_address", reinterpret_cast<char *>(&address), 4);
}

void nRF905::set_tx_address(uint32_t address) {
    LOG("Set TX address: " << std::hex << address << std::dec);
    set_attribute("tx_address", reinterpret_cast<char *>(&address), 4);
}

void nRF905::set_listen() {
    char c = '1';
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

void nRF905::send(const std::string& data) {
    char buf[32] {};
    strncpy(buf, data.c_str(), 32);
    if (write(fd, buf, 32) == -1) {
        throw IOException("Failed to write to nRF905 device", errno);
    }
    LOG_DETAILED("Sent: " << buf);
}

std::string nRF905::receive() {
    char buf[32] {};
    if (read(fd, buf, 32) == -1) {
        throw IOException("Failed to read from nRF905 device", errno);
    }
    LOG_DETAILED("Received: " << buf);
    return buf;
}
