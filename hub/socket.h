#pragma once

#include <string>
#include <iostream>
#include <sys/un.h>
#include "poller.h"

class Socket {
private:
    enum { LISTENING, CONNECTED } state;
    Poller& poller;
    int sock_fd;

    void remove_connection();
    void add_connection(const std::function<void(void)>& recv_handler);

public:
    int comm_fd;
    Socket(const std::string& filename, Poller& poller, const std::function<void(void)>& recv_handler);
    ~Socket();

    bool connected();
    std::string receive();
    void send(const std::string& data);
};
