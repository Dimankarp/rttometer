#include <cstdio>
#include <iostream>
#include <print>
#include <sys/socket.h>
#include "sys/types.h"
#include "sys/socket.h"
#include <netinet/in.h>

const static int UDP_PROTOCOL = 17;

int main(){
    std::print("This is gonna be an RTTometer\n");

    auto udp = socket(AF_INET, SOCK_DGRAM, UDP_PROTOCOL);
    if (udp == -1){
        perror("Failed to open udp socket");
        exit(1);
    }
    struct sockaddr_in addr {.sin_family=AF_INET, .sin_port=htons(6556), .sin_addr={.s_addr=INADDR_ANY}, .sin_zero={}} ;
    auto ret = bind(udp, (struct sockaddr*)&addr, sizeof(addr));
    if(ret != 0){
        perror("Failed to bind to port");
        exit(1);
    }
    char buf[256];
    recv(udp, buf, sizeof(buf), 0);
    std::cout << std::string{buf};
}