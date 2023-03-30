#include<stdio.h>
#include<sys/socket.h>
#include <arpa/inet.h>

int main(){
    printf("Hello World!!\n");
    char ip4[INET_ADDRSTRLEN]; // space to hold the IPvstring
    struct sockaddr_in sa; // pretend this is loaded with something

    inet_ntop(AF_INET, &(sa.sin_addr), ip4, INET_ADDRSTRLEN);

    printf("The IPv address is: %s\n", ip4);


    // IPv6:

    char ip6[INET6_ADDRSTRLEN]; // space to hold the IPvstring
    struct sockaddr_in6 sa_6;

    inet_ntop(AF_INET6, &(sa_6.sin6_addr), ip6, INET6_ADDRSTRLEN);

    printf("The IPv6 address is: %s\n", ip6);

    return 0;
}