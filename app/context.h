#pragma once


#include <netinet/in.h>
#include <optional>
enum class LaunchType{
    CLIENT, SERVER
};

struct Context {
    LaunchType type = LaunchType::CLIENT;
    std::optional<in_addr> ip;
    std::optional<in_port_t> port;
};