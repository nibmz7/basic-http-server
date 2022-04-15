#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <getopt.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>     // for fcntl()
#include <sys/epoll.h> // for epoll_create1(), epoll_ctl(), struct epoll_event

#define MAX_EVENTS 10

int main(int argc, char *argv[])
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int listen_sd, status, yes = 1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Stream socket */
    hints.ai_flags = AI_PASSIVE;     /* For wildcard IP address */
    hints.ai_protocol = IPPROTO_TCP; /* TCP */

    /* getaddrinfo() returns a list of address structures.
        Try each address until we successfully bind(2).
        If socket(2) (or bind(2)) fails, we (close the socket
        and) try the next address. */
    if ((status = getaddrinfo(NULL, "80", &hints, &result)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 2;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        listen_sd = socket(rp->ai_family, rp->ai_socktype,
                           rp->ai_protocol);
        if (listen_sd == -1)
            continue;

        // lose the pesky "Address already in use" error message
        if (setsockopt(listen_sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1)
            continue;

        // Set socket to non-blocking
        int flags = fcntl(listen_sd, F_GETFL, 0);
        if (fcntl(listen_sd, F_SETFL, flags | O_NONBLOCK))
            continue;

        if (bind(listen_sd, rp->ai_addr, rp->ai_addrlen) == 0)
            break; /* Success */

        close(listen_sd);
    }

    freeaddrinfo(result); /* No longer needed */

    if (rp == NULL)
    { /* No address succeeded */
        fprintf(stderr, "Could not bind\n");
        exit(EXIT_FAILURE);
    }

    if (listen(listen_sd, 32))
    {
        perror("listen() failed");
        close(listen_sd);
        exit(-1);
    }

    struct epoll_event ev, events[MAX_EVENTS];
    int client_sock, nfds, epollfd, n;
    struct sockaddr_storage client_addr; // connector's address information
    socklen_t addr_size;

    addr_size = sizeof client_addr;

    // Create epoll file descriptor
    epollfd = epoll_create1(0);
    if (epollfd == -1)
    {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    // Add listener socket for epoll to watch
    ev.events = EPOLLIN;
    ev.data.fd = listen_sd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sd, &ev) == -1)
    {
        perror("epoll_ctl: listen_sd");
        exit(EXIT_FAILURE);
    }

    for (;;)
    {
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1)
        {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (n = 0; n < nfds; ++n)
        {
            if (events[n].data.fd == listen_sd)
            {
                client_sock = accept(listen_sd, (struct sockaddr *)&client_addr, &addr_size);
                if (client_sock == -1)
                {
                    perror("accept");
                    exit(EXIT_FAILURE);
                }
                // Add client socket for epoll to watch
                ev.events = EPOLLOUT;
                ev.data.fd = client_sock;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, client_sock,
                              &ev) == -1)
                {
                    perror("epoll_ctl: client_sock");
                    exit(EXIT_FAILURE);
                }
                printf("Client added %d\n", client_sock);
            }
            else
            {
                // Not sure how to move on from here yet...
                printf("Socker ready %d\n", events[n].data.fd);
                close(events[n].data.fd);
            }
        }
    }

    return 0;
}