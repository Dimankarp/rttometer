#include "server.h"
#include "abort.h"
#include "context.h"
#include <arpa/inet.h>
#include <csignal>
#include <cstdio>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
namespace rtt {

namespace {
int socket_to_close_fd = -1;
void server_sig_handler(int /*signum*/) {
    std::print(std::cerr, "Interrupted\n");
    close(socket_to_close_fd);
    exit(0);
}
const int BUFF_SZ = 255;
} // namespace

void server_routine(const rtt::Context& ctx) {


    auto udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(udp == -1) {
        rtt::perror_abort("Failed to create socket\n");
    }
    socket_to_close_fd = udp;

    struct sigaction sa{};
    sa.sa_handler = server_sig_handler;
    sigaction(SIGINT, &sa, nullptr);


    struct sockaddr_in addr{ .sin_family = AF_INET,
                             .sin_port = htons(ctx.port.value()),
                             .sin_addr = { .s_addr = INADDR_ANY },
                             .sin_zero = {} };
    auto ret = bind(udp, (struct sockaddr*)&addr, sizeof(addr));
    if(ret != 0) {
        rtt::perror_abort("Failed to bind socket to port {}\n", ctx.port.value());
    }
    std::array<char, BUFF_SZ> buf{};
    std::print("Server started\n");
    for(;;) {
        struct sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);
        auto ret = recvfrom(udp, buf.data(), buf.size(), 0,
                            (struct sockaddr*)&client_addr, &addr_len);
        if(ret == -1)
            rtt::perror_abort("Failed to receive msg\n");

        buf[ret] = 0;
        if(ctx.is_verbose)
            std::print("Sending back to {}:{}\n", inet_ntoa(client_addr.sin_addr),
                       ntohs(client_addr.sin_port));
        sendto(udp, buf.data(), ret, 0, (struct sockaddr*)&client_addr, addr_len);
    }
}
} // namespace rtt