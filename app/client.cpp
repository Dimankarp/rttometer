#include "client.h"
#include "abort.h"
#include "print"
#include <arpa/inet.h>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <iostream>
#include <netinet/in.h>
#include <optional>
#include <sys/epoll.h>
#include <sys/timerfd.h>
namespace rtt {
namespace {
int socket_to_close_fd = -1;
void client_sig_handler(int /*signum*/) {
    std::print(std::cerr, "Interrupted\n");
    close(socket_to_close_fd);
    exit(0);
}

const unsigned long DEFAULT_PERIOD_MCS = 100000;
const int BUFF_SZ = 255;
const std::string handshake_msg = "hello";


struct Rtt {
    unsigned long start_mcs;
    unsigned long rtt_mcs;
};

void client_handshake(int sock, const sockaddr_in& addr) {

    auto ret = sendto(sock, handshake_msg.data(), handshake_msg.size(), 0,
                      (const sockaddr*)&addr, sizeof(addr));
    if(ret == -1)
        rtt::perror_abort("Failed to send handshake to {}\n", inet_ntoa(addr.sin_addr));

    std::array<char, BUFF_SZ> buf{};
    ret = recv(sock, buf.data(), buf.size(), 0);
    if(ret == -1)
        rtt::perror_abort("Failed to receive handshake from {} \n",
                          inet_ntoa(addr.sin_addr));
    if(ret != handshake_msg.size() || handshake_msg != buf.data())
        rtt::abort("Handshake msg malformed\n");
}

void send_time(int udp, const sockaddr_in& addr, auto now) {
    auto now_mcs =
    std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch())
    .count();
    sendto(udp, &now_mcs, sizeof(now_mcs), 0, (const sockaddr*)&addr, sizeof(addr));
}

Rtt receive_rtt(int udp, void* buf, std::size_t buf_sz) {
    auto ret = recv(udp, buf, buf_sz, 0);
    auto start_mcs = *(long*)buf;
    auto now = std::chrono::high_resolution_clock::now();
    auto now_mcs =
    std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch())
    .count();
    auto rtt = now_mcs - start_mcs;
    return { .start_mcs = static_cast<unsigned long>(start_mcs),
             .rtt_mcs = static_cast<unsigned long>(rtt) };
}
} // namespace


void client_routine(const rtt::Context& ctx) {

    std::ostream out(ctx.out_file.has_value() ? ctx.out_file.value().rdbuf() :
                                                std::cout.rdbuf());


    auto udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(udp == -1)
        rtt::perror_abort("Failed to create socket\n");


    socket_to_close_fd = udp;
    struct sigaction sa{};
    sa.sa_handler = client_sig_handler;
    sigaction(SIGINT, &sa, nullptr);


    const sockaddr_in addr{ .sin_family = AF_INET,
                            .sin_port = htons(ctx.port.value()),
                            .sin_addr = { .s_addr = ctx.ip.value().s_addr },
                            .sin_zero = {} };
    if(ctx.is_verbose)
        std::print("Sending handshake...\n");
    client_handshake(udp, addr);
    if(ctx.is_verbose)
        std::print("Successfully received handshake back...\n");

    std::chrono::microseconds period{ ctx.period_mcs.has_value() ?
                                      ctx.period_mcs.value() :
                                      DEFAULT_PERIOD_MCS };
    int tfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if(tfd == -1)
        rtt::perror_abort("Failed to create timerfd\n");
    itimerspec its{};
    its.it_interval.tv_nsec = static_cast<long>(period.count() * 1000);
    its.it_value = its.it_interval;
    if(timerfd_settime(tfd, 0, &its, nullptr) == -1)
        rtt::perror_abort("Failed to set timerfd\n");


    int epoll_fd = epoll_create1(0);
    if(epoll_fd == -1)
        rtt::perror_abort("Failed to create epoll\n");

    epoll_event ev{ .events = EPOLLIN, .data = { .fd = udp } };
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, udp, &ev);
    epoll_event tev{ .events = EPOLLIN, .data = { .fd = tfd } };
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, tfd, &tev);

    epoll_event events[2];

    const bool is_limited = ctx.period_mcs.has_value();
    unsigned long received = 0;

    std::array<char, BUFF_SZ> buf{};

    for(;;) {
        int fd_count = epoll_wait(epoll_fd, events, 2, -1);
        if(fd_count < 0)
            rtt::abort("Failed to wait for udp packet\n");

        for(int i = 0; i < fd_count; i++) {
            int fd = events[i].data.fd;
            if(fd == tfd) {
                uint64_t expirations = 0;
                if(read(tfd, &expirations, sizeof(expirations)) != sizeof(expirations)) {
                    rtt::abort("Failed to read timerfd\n");
                }
                for(uint64_t k = 0; k < expirations; ++k) {
                    auto now = std::chrono::high_resolution_clock::now();
                    send_time(udp, addr, now);
                }
            } else if(fd == udp) {
                auto rtt = receive_rtt(udp, buf.data(), buf.size());
                if(ctx.is_verbose)
                    std::print("Start: {} | rtt: {}\n", rtt.start_mcs, rtt.rtt_mcs);

                out << rtt.start_mcs << " " << rtt.rtt_mcs << "\n";
                if(is_limited)
                    received++;
            }
        }
        if(is_limited && received == ctx.requests_num.value())
            break;
    }
}
} // namespace rtt