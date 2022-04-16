#include <errno.h>
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

// - source file descriptor
// - \*method
// - \*uri
// - \*header_field
//   - key (strdup)
//   - value (strdup)
// - header_size
// - \*number of bytes sent

typedef struct header_field
{
    const char *name;
    const char *value;
    const struct header_field *next;
} header_field_t;

typedef struct
{
    int fd;
    char *method;
    char *uri;
    const header_field_t *header_field;
} client_conn_data_t;

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

    // getaddrinfo() returns a list of address structures.
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

    struct epoll_event ev;
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
    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = NULL;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sd, &ev) == -1)
    {
        perror("epoll_ctl: listen_sd");
        exit(EXIT_FAILURE);
    }

    struct epoll_event *events = calloc(MAX_EVENTS, sizeof(ev));
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
            if ((events[n].events & EPOLLERR) || (events[n].events & EPOLLHUP))
            {
                // error case
                fprintf(stderr, "epoll error\n");
                // close(events[n].data.fd);
                continue;
            }
            if (events[n].data.ptr == NULL)
            {

                client_sock = accept(listen_sd, (struct sockaddr *)&client_addr, &addr_size);
                if (client_sock == -1)
                {
                    perror("accept");
                    exit(EXIT_FAILURE);
                }
                set_nonblocking(client_sock);

                client_conn_data_t *ptr;
                ptr = (client_conn_data_t *)calloc(1, sizeof(client_conn_data_t));

                if (ptr == NULL)
                {
                    printf("Memory allocation failed");
                    exit(1); // exit the program
                }

                ptr->fd = client_sock;
                // Add client socket for epoll to watch
                ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
                ev.data.ptr = ptr;
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
                if (((events[n].events & EPOLLIN)))
                {
                    // client socket; read as much data as we can
                    char buf[1024];
                    const int client_fd = ((client_conn_data_t *)events[n].data.ptr)->fd;
                    for (;;)
                    {
                        ssize_t nbytes = read(client_fd, buf, sizeof(buf));
                        if (nbytes == -1)
                        {
                            if (errno == EAGAIN || errno == EWOULDBLOCK)
                            {
                                printf("finished reading data from client\n");
                                break;
                            }
                            else
                            {
                                perror("read()");
                                return 1;
                            }
                        }
                        else if (nbytes == 0)
                        {
                            printf("finished with %d\n", client_fd);
                            close(client_fd);
                            break;
                        }
                        else
                        {
                            int i;
                            int first_line = 1;
                            for (i = 0; i < nbytes; i++)
                            {
                                if (buf[i] == '\n')
                                {
                                    if (first_line)
                                    {
                                        char *method, *uri, *version;
                                        sscanf(buf, "%ms %ms %ms\n", &method, &uri, &version);
                                        ((client_conn_data_t *)events[n].data.ptr)->uri = uri;
                                        ((client_conn_data_t *)events[n].data.ptr)->method = method;
                                    }
                                    else
                                    {
                                    }
                                    first_line = 0;
                                }
                            }

                            printf("%s %s\n", ((client_conn_data_t *)events[n].data.ptr)->method, ((client_conn_data_t *)events[n].data.ptr)->uri);
                        }
                    }
                }
            }
        }
    }

    return 0;
}