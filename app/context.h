#pragma once


#include <fstream>
#include <netinet/in.h>
#include <optional>
namespace rtt {
enum class LaunchType { CLIENT, SERVER };

struct Context {
    LaunchType type = LaunchType::CLIENT;
    std::optional<in_addr> ip;
    std::optional<in_port_t> port;
    std::optional<std::ofstream> out_file;
    std::optional<unsigned long> requests_num;
    std::optional<unsigned long> period_mcs;
    bool is_verbose = false;
};
} // namespace rtt