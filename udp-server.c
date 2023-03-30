#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "3490" // the port users will be connecting to

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main()
{
    // sockfd -> file descriptor responsible opening a socket for the server port
    // new_fd -> new socket that will be used to send message to client
    int sockfd, new_fd, yes = 1;
    // hints -> structure providing hints like port number, ip protocol and hostname to getaddrinfo
    // servinfo -> structure returned by getaddrinfo (a linked list for possible candidates for binding)
    // iter -> to iterate servinfo
    struct addrinfo hints, *servinfo, *iter;
    // clear the hints structure
    memset(&hints, 0, sizeof hints);
    // set the hints for getting usable candidates
    // set addressinfo family to be unspecified -> can be ipv4 or ipv6 for all we care
    hints.ai_family = AF_INET6;
    // use sock type to sock_stream -> transport protocol used will be TCP (other options can include UDP as well)
    hints.ai_socktype = SOCK_DGRAM;

    hints.ai_flags = AI_PASSIVE;
    int getaddrinfo_status;
    // Function signature
    //  int getaddrinfo(const char *restrict node,   -> "hostname"
    // const char *restrict service,             -> PORT
    // const struct addrinfo *restrict hints,    -> hints
    // struct addrinfo **restrict res);          -> pointer to where you want the addrinfo for candidates to be stored
    if ((getaddrinfo_status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)
    {
        // The gai_strerror() function translates these error codes to a
        // human readable string, suitable for error reporting.
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(getaddrinfo_status));
        return 1;
    }
    // loop through all the results and bind to the first we can
    for (iter = servinfo; iter != NULL; iter = iter->ai_next)
    {
        if ((sockfd = socket(iter->ai_family, iter->ai_socktype, iter->ai_protocol)) == -1)
        {
            perror("server: socket");
            continue;
        }

        // set socket option SO_REUSEADDR to 1 which allows to bind to a hanging socket
        // if there is no atcive connection
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            perror("Not able to set socket option");
            continue;
        }

        if (bind(sockfd, iter->ai_addr, iter->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    // no use of servinfo as we have allocated an address for the port
    // if iter is null, no addrinfo found suitable
    if (iter == NULL)
    {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    freeaddrinfo(servinfo);
    struct sockaddr_storage client_addr;
    socklen_t addr_size = sizeof client_addr;
    char client_formatted_address[INET6_ADDRSTRLEN];

    int numbytes;
    char buf[100];
    if ((numbytes = recvfrom(sockfd, buf, sizeof(buf) - 1, 0, 
        (struct sockaddr *)&client_addr, &addr_size)) == -1)
    {
        perror("Not able to recieve");
        exit(1);
    }
    inet_ntop(client_addr.ss_family,
                                        get_in_addr((struct sockaddr *)&client_addr),
                                        client_formatted_address,
                                        sizeof(client_formatted_address));
    printf("Got packet from %s for %d bytes\n", client_formatted_address, numbytes);

    buf[numbytes] = '\0';
    printf("%s\n", buf);
    close(sockfd);

    return 0;
}