#pragma once

#include <string.h>
#include <exception>
#include <string>

struct IOException : public std::exception {
    IOException(const std::string& what, int err_no) : what(what + ": " + strerror(err_no)) {}
    std::string what;
};
