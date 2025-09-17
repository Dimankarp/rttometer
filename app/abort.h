#pragma once
#include "format"
#include "string"
#include <print>

namespace rtt {

    template <typename... Args>
void abort(std::format_string<Args...> fmt, Args&&... args) {
    std::print(stderr, fmt, std::forward<decltype(args)>(args)...);
    exit(1);
}
 template <typename... Args>
void perror_abort(std::format_string<Args...> fmt, Args&&... args) {
    auto msg = std::format(fmt, std::forward<decltype(args)>(args)...);
    perror(msg.data());
    exit(1);
}

}


