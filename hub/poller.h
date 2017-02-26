#pragma once

#include <map>
#include <functional>

class Poller {
private:
    std::map<int, std::function<void(void)>> handlers;

public:
    void add_handler(int fd, const std::function<void(void)>& handler);
    void remove_handler(int fd);
    void run();
};
