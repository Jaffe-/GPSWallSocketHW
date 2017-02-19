#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <string>
#include <iostream>
#include "poller.h"
#include "socket.h"
#include "ioexception.h"
#include "log.h"

Socket::Socket(const std::string& filename, Poller& poller, const std::function<void(void)>& recv_handler)
    : state(LISTENING),
      poller(poller) {
    if ((sock_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        throw IOException("Failed to create socket", errno);
    }

    struct sockaddr_un local;
    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, filename.c_str());
    unlink(local.sun_path);
    int len = strlen(local.sun_path) + sizeof(local.sun_family);
    if (bind(sock_fd, (struct sockaddr *)&local, len) == -1) {
        throw IOException("Failed to bind to socket on " + filename, errno);
    }

    if (listen(sock_fd, 1) == -1) {
        throw IOException("Failed to listen on socket", errno);
    }

    chmod(local.sun_path, 0777);
    poller.add_handler(sock_fd, [=]() { add_connection(recv_handler); });
    LOG("Socket initialized, listening for connection");
}

Socket::~Socket() {
    LOG("Closing socket");
    close(sock_fd);
}

bool Socket::connected() {
    return state == CONNECTED;
}

std::string Socket::receive() {
    char buf[4096] {};
    int read_size;
    if ((read_size = read(comm_fd, buf, sizeof(buf))) == -1) {
        remove_connection();
        return "";
    }
    buf[read_size] = '\0';
    LOG_DETAILED("Socket received: " << buf);
    return buf;
}

void Socket::send(const std::string& data) {
    if (write(comm_fd, data.c_str(), data.length()) == -1) {
        remove_connection();
        return;
    }
    LOG_DETAILED("Socket sent: " << data);
}

void Socket::add_connection(const std::function<void(void)>& recv_handler) {
    struct sockaddr_un remote;
    unsigned int rsize = sizeof(remote);

    if (state == LISTENING) {
        if ((comm_fd = accept(sock_fd, (struct sockaddr *)&remote, &rsize)) == -1) {
            throw IOException("Failed to accept() on socket", errno);
        }
        LOG("New connection");
        poller.add_handler(comm_fd, recv_handler);
        state = CONNECTED;
    }
    else {
        int new_fd;
        if ((new_fd = accept(sock_fd, (struct sockaddr *)&remote, &rsize)) == -1) {
            throw IOException("Failed to accept() on socket", errno);
        }
        close(new_fd);
    }
}

void Socket::remove_connection() {
    LOG("Removed connection");
    poller.remove_handler(comm_fd);
    state = LISTENING;
}
