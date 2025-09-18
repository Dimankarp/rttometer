#include "abort.h"
#include "context.h"
#include "sys/socket.h"
#include <arpa/inet.h>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <iostream>
#include <limits>
#include <netinet/in.h>
#include <print>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>
#include <unistd.h>

const static int UDP_PROTOCOL = 17;

namespace {
void server_sig_handler(int /*signum*/) {
    std::print("Interrupted");
    exit(0);
}

using timepoint = decltype(std::chrono::high_resolution_clock::now());

// std::vector<st>
void client_sig_handler(int /*signum*/) {
    std::print("Interrupted");
    exit(0);
}

const std::string handshake_msg = "hello";
void client_handshake(int sock, const struct sockaddr_in& addr) {

    auto ret = sendto(sock, handshake_msg.data(), handshake_msg.size(), 0,
                      (const struct sockaddr*)&addr, sizeof(addr));
    if(ret == -1)
        rtt::perror_abort("Failed to send handshake to {}\n", inet_ntoa(addr.sin_addr));

    std::array<char, 256> buf{};
    ret = recv(sock, buf.data(), buf.size(), 0);
    if(ret == -1)
        rtt::perror_abort("Failed to receive handshake from {} \n",
                          inet_ntoa(addr.sin_addr));
    if(ret != handshake_msg.size() || handshake_msg != buf.data())
        rtt::abort("Handshake msg malformed\n");
}

struct sockaddr_in server_handshake(int sock, const struct sockaddr_in& addr) {

    std::array<char, 256> buf{};
    struct sockaddr_in client_addr{};
    socklen_t addr_len = sizeof(client_addr);
    auto ret = recvfrom(sock, buf.data(), buf.size(), 0,
                        (struct sockaddr*)&client_addr, &addr_len);
    if(ret == -1)
        rtt::perror_abort("Failed to receive handshake from {} \n",
                          inet_ntoa(addr.sin_addr));
    if(ret != handshake_msg.size() || handshake_msg != buf.data())
        rtt::abort("Handshake msg malformed");
    ret = sendto(sock, handshake_msg.data(), handshake_msg.size(), 0,
                 (struct sockaddr*)&client_addr, sizeof(client_addr));
    if(ret == -1)
        rtt::perror_abort("Failed to send handshake to {}", inet_ntoa(addr.sin_addr));
    return client_addr;
}


void client_routine(const rtt::Context& ctx) {


    struct sigaction sa{};
    sa.sa_handler = client_sig_handler;
    sigaction(SIGINT, &sa, nullptr);


    auto udp = socket(AF_INET, SOCK_DGRAM, UDP_PROTOCOL);
    if(udp == -1)
        rtt::perror_abort("Failed to create socket\n");


    const struct sockaddr_in addr{ .sin_family = AF_INET,
                                   .sin_port = htons(ctx.port.value()),
                                   .sin_addr = { .s_addr = ctx.ip.value().s_addr },
                                   .sin_zero = {} };

    client_handshake(udp, addr);


    static const char* text = "hello\n";
    std::array<char, 255> buf{};
    int epoll_fd = epoll_create1(0);
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = udp;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, udp, &ev);
    struct epoll_event event;
    timepoint next_send_time = std::chrono::high_resolution_clock::now();
    std::chrono::microseconds period{ 100000 };

    for(;;) {
        auto now = std::chrono::high_resolution_clock::now();
        if(now >= next_send_time) {
            auto now_mcs =
            std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch())
            .count();
            sendto(udp, &now_mcs, sizeof(now_mcs), 0,
                   (const struct sockaddr*)&addr, sizeof(addr));
            next_send_time = now + period;
        }
        auto timeout = next_send_time - now;
        int count = epoll_wait(
        epoll_fd, &event, 1,
        std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count());
        if(count != 0) {
            auto ret = recv(udp, buf.data(), buf.size(), 0);
            buf[ret] = 0;
            auto start_mcs = *(long*)buf.data();
            now = std::chrono::high_resolution_clock::now();
            auto now_mcs =
            std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch())
            .count();
            auto rtt = now_mcs - start_mcs;
            std::cout << std::to_string(rtt) << "\n";
        }
    }
}


void server_routine(const rtt::Context& ctx) {

    struct sigaction sa{};
    sa.sa_handler = server_sig_handler;
    sigaction(SIGINT, &sa, nullptr);

    auto udp = socket(AF_INET, SOCK_DGRAM, UDP_PROTOCOL);
    if(udp == -1) {
        rtt::perror_abort("Failed to create socket\n");
    }

    struct sockaddr_in addr{ .sin_family = AF_INET,
                             .sin_port = htons(ctx.port.value()),
                             .sin_addr = { .s_addr = INADDR_ANY },
                             .sin_zero = {} };
    auto ret = bind(udp, (struct sockaddr*)&addr, sizeof(addr));
    if(ret != 0) {
        rtt::perror_abort("Failed to bind socket to port {}\n", ctx.port.value());
    }
    std::array<char, 255> buf{};

    for(;;) {
        struct sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);
        auto ret = recvfrom(udp, buf.data(), buf.size(), 0,
                            (struct sockaddr*)&client_addr, &addr_len);
        if(ret == -1)
            rtt::perror_abort("Failed to receive msg\n");

        buf[ret] = 0;
        std::cout << std::string{ buf.data(), 0, static_cast<size_t>(ret) } << "\n";
        sendto(udp, buf.data(), ret, 0, (struct sockaddr*)&client_addr, addr_len);
    }
}
} // namespace

static const char* HELP_STR =
"Usage: rttometer [-sh] (ip) port\n"
"-s: Launch in server-mode (client mode by default)\n"
"-h: Show this message\n"
"(ip): Server IPv4 address. Mandatory for client-mode.\n"
"port: Server port. Mandatory.\n";

int main(int argc, char* const* argv) {
    std::print("This is gonna be an RTTometer\n");
    rtt::Context ctx{};

    int opt;
    while((opt = getopt(argc, argv, "sh")) != -1) {
        switch(opt) {
        case 's': ctx.type = rtt::LaunchType::SERVER; break;
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
        if(!inet_aton(argv[args_ind], &addr))
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
