#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>  // stderr
#include <stdlib.h> // EXIT_FAILURE
#include <unistd.h>
#include <errno.h>
#include <string.h>    //memset
#include <sys/epoll.h> // for epoll_create1(), epoll_ctl(), struct epoll_event
#include "http.h"

static void set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        perror("fcntl()");
        return;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        perror("fcntl()");
    }
}

int create_listening_socket(char *port, int epollfd)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int listen_sd, status, yes = 1;
    struct epoll_event ev;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Stream socket */
    hints.ai_flags = AI_PASSIVE;     /* For wildcard IP address */
    hints.ai_protocol = IPPROTO_TCP; /* TCP */

    // getaddrinfo() returns a list of address structures.
    if ((status = getaddrinfo(NULL, port, &hints, &result)) != 0)
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

        set_nonblocking(listen_sd);

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

    // Add listener socket for epoll to watch
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = listen_sd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sd, &ev) == -1)
    {
        perror("epoll_ctl: listen_sd");
        exit(EXIT_FAILURE);
    }

    return listen_sd;
}

void accept_connections(int listen_sd, int epollfd)
{
    int client_sock;
    struct sockaddr_storage client_addr; // connector's address information - not in use
    socklen_t addr_size = sizeof client_addr;
    struct epoll_event ev;

    for (;;)
    {
        client_sock = accept(listen_sd, (struct sockaddr *)&client_addr, &addr_size);
        if (client_sock == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // we processed all of the connections
                break;
            }
            perror("accept");
            exit(EXIT_FAILURE);
        }
        set_nonblocking(client_sock);

        client_conn_data_t *ptr = calloc(1, sizeof(client_conn_data_t));

        if (ptr == NULL)
        {
            perror("client_conn_data_t calloc()");
            exit(EXIT_FAILURE);
        }

        ptr->fd = client_sock;
        ptr->uri = NULL;
        ptr->offset = 0;
        ptr->finished_reading = 0;

        // Add client socket for epoll to watch
        ev.events = EPOLLIN | EPOLLET;
        ev.data.ptr = ptr;
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, client_sock, &ev) == -1)
        {
            perror("epoll_ctl: client_sock");
            exit(EXIT_FAILURE);
        }
        printf("Client added %d\n", client_sock);
    }
}
