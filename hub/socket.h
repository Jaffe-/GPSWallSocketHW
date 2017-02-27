#pragma once

#include <string>
#include <iostream>
#include <vector>
#include "json.hpp"

using json = nlohmann::json;

class Socket {
private:
    int fd;

public:
    Socket(const std::string& filename);
    ~Socket();

    int get_fd() const { return fd; }
    std::vector<json> receive();
    void send(const json& data);
};
