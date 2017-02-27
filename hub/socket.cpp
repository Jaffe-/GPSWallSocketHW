#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <string>
#include <iostream>
#include <vector>
#include <sstream>
#include "json.hpp"
#include "socket.h"
#include "ioexception.h"

#define LOG_MODULE "Socket"
#include "log.h"

Socket::Socket(const std::string& filename) {
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        throw IOException("Failed to create socket", errno);
    }

    struct sockaddr_un remote;
    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, filename.c_str());
    int len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    if (connect(fd, (struct sockaddr *)&remote, len) == -1) {
        throw IOException("Failed to connect to node app", errno);
    }

    LOG("Connected to node app");
}

Socket::~Socket() {
    LOG("Closing socket");
    close(fd);
}

std::vector<json> Socket::receive() {
    char buf[4096] {};
    int read_size;
    if ((read_size = read(fd, buf, sizeof(buf))) == -1) {
        throw IOException("Failed to read from node socket", errno);
    }
    buf[read_size] = '\0';
    if (read_size > 0) {
        LOG_DETAILED("Received: " << buf);
    }

    std::vector<json> json_packets;
    std::stringstream ss(buf);
    std::string line;
    while (std::getline(ss, line)) {
        json_packets.push_back(json::parse(line));
    }
    return json_packets;
}

void Socket::send(const json& data) {
    std::string msg = data.dump() + '\n';
    LOG("send: " << msg);
    if (write(fd, msg.c_str(), msg.length()) == -1) {
        throw IOException("Failed to write to node socket", errno);
    }
    LOG_DETAILED("Sent: " << data);
}
