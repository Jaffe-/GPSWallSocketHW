#include <sys/select.h>
#include <functional>
#include <iostream>
#include "poller.h"
#include "ioexception.h"

#define LOG_MODULE "Poller"
#include "log.h"

void Poller::add_handler(int fd, const std::function<void(void)>& handler) {
    LOG_DETAILED("Adding handler for fd " << fd);
    handlers[fd] = handler;
}

void Poller::remove_handler(int fd) {
    auto it = handlers.find(fd);
    if (it != handlers.end()) {
        LOG_DETAILED("Removing handler for fd " << fd);
        handlers.erase(it);
    }
}

void Poller::run() {
    fd_set readfds;
    int max_fd = 0;

    FD_ZERO(&readfds);
    for (const auto& entry : handlers) {
        int fd = entry.first;
        FD_SET(fd, &readfds);
        if (fd > max_fd) {
            max_fd = fd;
        }
    }

    if (select(max_fd + 1, &readfds, NULL, NULL, NULL) == -1) {
        throw IOException("select() failed", errno);
    }

    for (const auto& entry : handlers) {
        int fd = entry.first;
        auto fn = entry.second;

        if (FD_ISSET(fd, &readfds)) {
            fn();
        }
    }
}
