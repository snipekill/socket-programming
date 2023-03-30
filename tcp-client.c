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

int main(int argc, char *argv[])
{
    if(argc != 2){
        fprintf(stderr, "Usage ./a.out hostname");
        exit(1);
    }

    int sockfd, new_fd, yes = 1;
    struct addrinfo hints, *servinfo, *iter;
    // clear the hints structure
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    int getaddrinfo_status;
    // Function signature
    //  int getaddrinfo(const char *restrict node,   -> "hostname"
    // const char *restrict service,             -> PORT
    // const struct addrinfo *restrict hints,    -> hints
    // struct addrinfo **restrict res);          -> pointer to where you want the addrinfo for candidates to be stored
    if ((getaddrinfo_status = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0)
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

        if (connect(sockfd, iter->ai_addr, iter->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("Not able to connect");
            continue;
        }

        break;
    }

    // no use of servinfo as we have allocated an address for the port
    // if iter is null, no addrinfo found suitable
    if (iter == NULL)
    {
        fprintf(stderr, "server: failed to connect\n");
        exit(1);
    }

    char server_formatted_address[INET6_ADDRSTRLEN];

    inet_ntop(iter->ai_family,
                get_in_addr((struct sockaddr *)&iter->ai_addr),
                server_formatted_address, sizeof(server_formatted_address));
    printf("client: got connected to  %s\n", server_formatted_address);

    freeaddrinfo(servinfo);
    char buf[100];
    int numbytes;
    if((numbytes = recv(sockfd, buf, sizeof(buf), 0)) == -1){
        close(sockfd);
        perror("Not able to receive");
        exit(1);
    }
    buf[numbytes] = '\0';
    printf("client: received '%s'\n",buf);
    close(sockfd);
    
    

    return 0;
}