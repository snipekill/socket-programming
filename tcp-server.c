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

#define BACKLOG 10 // how many pending connections queue will hold

// handler for reaping dead child processes that we are invoking to send hello world to connecting clients

void sigchld_handler(int s)
{
    // wait for the child process to be reaped
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
    errno = saved_errno;
}

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
    hints.ai_family = AF_UNSPEC;
    // use sock type to sock_stream -> transport protocol used will be TCP (other options can include UDP as well)
    hints.ai_socktype = SOCK_STREAM;
    // If the AI_PASSIVE flag is specified in hints.ai_flags, and node
    // is NULL, then the returned socket addresses will be suitable for
    // bind(2)ing a socket that will accept(2) connections.  The
    // returned socket address will contain the "wildcard address"
    // (INADDR_ANY for IPv4 addresses, IN6ADDR_ANY_INIT for IPv6
    // address).  The wildcard address is used by applications
    // (typically servers) that intend to accept connections on any of
    // the host's network addresses.  If node is not NULL, then the
    // AI_PASSIVE flag is ignored.
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
        if ((sockfd = socket(iter->ai_family, iter->ai_socktype, iter->ai_protocol)) != 0)
        {
            perror("server: socket");
            continue;
        }

        // set socket option SO_REUSEADDR to 1 which allows to bind to a hanging socket
        // if there is no atcive connection
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) != 0)
        {
            perror("Not able to set socket option");
            continue;
        }

        if (bind(sockfd, iter->ai_addr, iter->ai_addrlen) != 0)
        {
            perror("Not able to bind to this address");
            continue;
        }

        break;
    }

    // no use of servinfo as we have allocated an address for the port
    freeaddrinfo(servinfo);
    // if iter is null, no addrinfo found suitable
    if (iter == NULL)
    {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    // let's listen to the port
    if (listen(sockfd, BACKLOG) != 0)
    {
        fprintf(stderr, "error in listening");
        exit(1);
    }

    struct sigaction sa;
    // set handler for sigchld signal
    sa.sa_handler = sigchld_handler;
    // clear the sig mask
    sigemptyset(&sa.sa_mask);
    // set flags
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &sa, NULL) != 0)
    {
        perror("Not able to set handler");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    socklen_t addr_size;
    struct sockaddr_storage client_addr;
    char client_formatted_address[INET6_ADDRSTRLEN];
    while (1)
    {
        addr_size = sizeof(client_addr);
        if ((new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_size)) != 0)
        {
            perror("Cannot accept this connection");
            continue;
        }

        inet_ntop(client_addr.ss_family,
                  get_in_addr((struct sockaddr *)&client_addr),
                  client_formatted_address, sizeof(client_formatted_address));
        printf("server: got connection from %s\n", client_formatted_address);

        pid_t child = fork();
        // in the child process
        if(child == 0){
            // close sockfd no use
            close(sockfd);
            if(send(new_fd, "Hello World", 12, 0) == -1) {
                perror("Not able to send hello");
                exit(0);
            }
        }
        else if(child > 0){
            close(new_fd);
        }
        else {
            fprintf(stderr, "Not able to spawn child");
            continue;
        }
    }

    return 0;
}