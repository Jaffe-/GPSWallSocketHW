#pragma once

#include <string>
#include <iostream>

class Socket {
private:
    int fd;

public:
    Socket(const std::string& filename);
    ~Socket();

    int get_fd() const { return fd; }
    std::string receive();
    void send(const std::string& data);
};
