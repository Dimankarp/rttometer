#include "abort.h"
#include "sys/socket.h"
#include "sys/types.h"
#include <arpa/inet.h>
#include <csignal>
#include <cstdio>
#include <iostream>
#include <netinet/in.h>
#include <print>
#include <string>
#include <sys/socket.h>

const static int UDP_PROTOCOL = 17;

namespace {
void server_sig_handler(int signum) {
    std::print("Interrupted");
    exit(0);
}

void client_sig_handler(int signum) {
    std::print("Interrupted");
    exit(0);
}

const std::string handshake_msg = "hello";
void client_handshake(int sock, const struct sockaddr_in& addr) {

    auto ret = sendto(sock, handshake_msg.data(), handshake_msg.size(), 0,
                      (const struct sockaddr*)&addr, sizeof(addr));
    if(ret == -1)
        rtt::perror_abort("Failed to send handshake to {}", inet_ntoa(addr.sin_addr));

    std::array<char, 256> buf{};
    ret = recv(sock, buf.data(), buf.size(), 0);
    if(ret == -1)
        rtt::perror_abort("Failed to receive handshake from {} \n",
                          inet_ntoa(addr.sin_addr));
    if(ret != handshake_msg.size() || handshake_msg != buf.data())
        rtt::abort("Handshake msg malformed");
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


void client_routine(in_addr ip, in_port_t port) {

    struct sigaction sa = { client_sig_handler };
    sigaction(SIGINT, &sa, nullptr);


    auto udp = socket(AF_INET, SOCK_DGRAM, UDP_PROTOCOL);
    if(udp == -1)
        rtt::perror_abort("Failed to create socket\n");


    const struct sockaddr_in addr{ .sin_family = AF_INET,
                                   .sin_port = htons(port),
                                   .sin_addr = { .s_addr = ip.s_addr },
                                   .sin_zero = {} };

    client_handshake(udp, addr);

    static const char* text = "hello\n";
    for(int i = 0; i < 10; i++)
        sendto(udp, text, 5, 0, (const struct sockaddr*)&addr, sizeof(addr));
}


void server_routine(in_port_t port) {

    struct sigaction sa = { server_sig_handler };
    sigaction(SIGINT, &sa, nullptr);

    auto udp = socket(AF_INET, SOCK_DGRAM, UDP_PROTOCOL);
    if(udp == -1) {
        rtt::perror_abort("Failed to create socket\n");
    }

    struct sockaddr_in addr{ .sin_family = AF_INET,
                             .sin_port = htons(port),
                             .sin_addr = { .s_addr = INADDR_ANY },
                             .sin_zero = {} };
    auto ret = bind(udp, (struct sockaddr*)&addr, sizeof(addr));
    if(ret != 0) {
        rtt::perror_abort("Failed to bind socket to port {}\n", port);
    }

    server_handshake(udp, addr);

    std::array<char, 255> buf{};
    for(;;) {
        auto ret = recv(udp, buf.data(), buf.size(), 0);
        if(ret == -1)
            rtt::perror_abort("Failed to receive msg\n");
        buf[ret] = 0;
        std::cout << std::string{ buf.data(), 0, static_cast<size_t>(ret) };
    }
}
} // namespace


int main(int argc, char** args) {
    std::print("This is gonna be an RTTometer\n");
    if(argc > 1 && std::string{ args[1] } == "-s")
        server_routine(6556);
    else {
        const char* ip = "127.0.0.1";
        struct in_addr addr{};
        inet_aton(ip, &addr);
        client_routine(addr, 6556);
    }
}
