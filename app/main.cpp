#include "abort.h"
#include "client.h"
#include "context.h"
#include "server.h"
#include <arpa/inet.h>
#include <iostream>
#include <limits>
#include <netinet/in.h>
#include <print>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>
#include <unistd.h>

static const std::string HELP_STR =
"Usage: rttometer [-sh] (ip) <port>\n"
"-s: Launch in server-mode (client mode by default)\n"
"-h: Show this message\n"
"-o <filename>: Output into file. Client-mode only\n"
"-n <num>: Do <num> measures and exit. Client-mode only\n"
"-p <num>: Client packet sending period in microseconds. Client-mode only\n"
"(ip): Server IPv4 address. Mandatory for client-mode\n"
"port: Server port. Mandatory\n";

int main(int argc, char* const* argv) {
    rtt::Context ctx{};

    int opt = 0;
    while((opt = getopt(argc, argv, "sho:n:p:v")) != -1) {
        switch(opt) {
        case 's': ctx.type = rtt::LaunchType::SERVER; break;
        case 'v': ctx.is_verbose = true; break;
        case 'o': {
            std::ofstream file(optarg);
            if(!file.good())
                rtt::abort("Failed to open file {}\n", optarg);
            ctx.out_file = std::move(file);
            break;
        }
        case 'n': {
            try {
                ctx.requests_num = std::stoul(optarg);
            } catch(std::invalid_argument& e) {
                rtt::abort("Failed to parse requests num");
            }
            break;
        }
        case 'p': {
            try {
                ctx.period_mcs = std::stoul(optarg);
            } catch(std::invalid_argument& e) {
                rtt::abort("Failed to parse period");
            }
            break;
        }
        case 'h': {
            std::cout << HELP_STR;
            exit(0);
        }
        default: rtt::abort("Invalid option {}\n", opt); break;
        }
    }
    int args_ind = optind;

    if(ctx.type == rtt::LaunchType::CLIENT) {
        if(args_ind >= argc)
            rtt::abort("Server ip wasn't provided \n");
        struct in_addr addr{};
        if(inet_aton(argv[args_ind], &addr) == 0)
            rtt::abort("Failed to parse {} as IPv4 address\n", argv[args_ind]);
        ctx.ip = addr;
        args_ind++;
    }

    if(args_ind >= argc) {
        rtt::abort("Port wasn't provided \n");
    }

    try {
        auto parsed = std::stoul(argv[args_ind]);
        if(parsed > std::numeric_limits<uint16_t>::max())
            rtt::abort("Provided port must be in range [0:65535]\n");
        ctx.port = parsed;
    } catch(std::invalid_argument& e) {
        rtt::abort("Failed to parse port\n");
    }


    if(ctx.type == rtt::LaunchType::CLIENT)
        client_routine(ctx);
    else
        server_routine(ctx);
}
