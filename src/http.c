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
#include <fcntl.h>        // for fcntl()
#include <sys/epoll.h>    // for epoll_create1(), epoll_ctl(), struct epoll_event
#include <sys/sendfile.h> // for sendfile()
#include "http.h"

#define MAX_EVENTS 10

// - source file descriptor
// - \*method
// - \*uri
// - \*header_field
//   - key (strdup)
//   - value (strdup)
// - header_size
// - \*number of bytes sent

#define BUF_SIZE 4096 * 1000
#define RES_HEADER_SIZE 32000

typedef struct header_field
{
    const char *name;
    const char *value;
    const struct header_field *next;
} header_field_t;

typedef struct
{
    int fd;
    int source_fd;
    char *method;
    char *uri;
    char *version;
    int finished_reading;
    off_t *offset;
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
    option_t option;

    get_options(argc, argv, &option);
    setcwd(option.dir);
    printf("port: %d\ncwd: %s\n", option.port, cwd);

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
    ev.events = EPOLLIN;
    // ev.events = EPOLLIN | EPOLLET; TODO: Fix bug when using Edge triggered polling
    ev.data.fd = listen_sd;
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
        printf("\nepoll event: %d\n", nfds);
        for (n = 0; n < nfds; ++n)
        {
            if ((events[n].events & EPOLLERR) || (events[n].events & EPOLLHUP))
            {
                // error case
                fprintf(stderr, "epoll error\n");
                /**
                 * TODO: Get correct file descriptor and free allocated memory if required
                 */
                // close(events[n].data.fd);
                continue;
            }

            if (events[n].data.fd == listen_sd)
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
                ptr->uri = NULL;
                ptr->offset = 0;
                ptr->finished_reading = 0;
                // Add client socket for epoll to watch
                // ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
                ev.events = EPOLLIN | EPOLLOUT;
                ev.data.ptr = ptr;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, client_sock, &ev) == -1)
                {
                    perror("epoll_ctl: client_sock");
                    exit(EXIT_FAILURE);
                }
                printf("Client added %d\n", client_sock);
            }
            else
            {
                // Read/write from/to client socket
                client_conn_data_t *conn = events[n].data.ptr;
                if (((events[n].events & EPOLLIN)))
                {

                    // client socket; read as much data as we can
                    /**
                     * TODO: Parse all header fields and handle blocking read
                     */
                    char buf[1024];
                    printf("Reading from client %d\n", conn->fd);
                    for (;;)
                    {
                        ssize_t nbytes = read(conn->fd, buf, sizeof(buf));
                        if (nbytes == -1)
                        {
                            if (errno == EAGAIN || errno == EWOULDBLOCK)
                            {
                                printf("blocked reading data from client %d\n", conn->fd);
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
                            conn->finished_reading = 1;
                            printf("finished reading from client %d\n", conn->fd);
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
                                    if (first_line && conn->uri == NULL)
                                    {
                                        printf("Read request info for %d\n", conn->fd);
                                        char *method, *uri, *version;
                                        sscanf(buf, "%ms %ms %ms\n", &method, &uri, &version);
                                        conn->uri = uri;
                                        conn->method = method;
                                        conn->version = version;
                                    }
                                    else
                                    {
                                        // Ignore for now
                                    }
                                    first_line = 0;
                                }
                            }
                        }
                    }
                }

                if (conn->uri != NULL)
                {
                    printf("Writing to client %d\n", conn->fd);
                    if (!conn->source_fd)
                    {
                        /**
                         * TODO: Get mime type
                         */
                        int offset;
                        file_info_t file_info;
                        char res_header_buf[RES_HEADER_SIZE];

                        if (get_static_file_info(conn->uri, &file_info) == -1)
                        {
                            offset = snprintf(res_header_buf, RES_HEADER_SIZE, "HTTP/1.1 404 Not found\r\n");
                        }
                        else
                        {
                            conn->source_fd = file_info.fd;
                            offset = snprintf(res_header_buf, RES_HEADER_SIZE, "HTTP/1.1 200 OK\r\n");
                            offset += snprintf(res_header_buf + offset, RES_HEADER_SIZE, "Server: Basic Web Server\r\n");
                            offset += snprintf(res_header_buf + offset, RES_HEADER_SIZE, "Content-length: %d\r\n", (int)file_info.size);
                            offset += snprintf(res_header_buf + offset, RES_HEADER_SIZE, "Content-type: %s\r\n", file_info.mime_type);
                        }
                        offset += snprintf(res_header_buf + offset, RES_HEADER_SIZE, "\r\n");

                        /**
                         * TODO: Handle blocking write (EAGAIN or EWOULDBLOCK)
                         */
                        if (write(conn->fd, res_header_buf, offset) == -1)
                        {
                            perror("write headers");
                            exit(EXIT_FAILURE);
                        }
                    }
                    int n = 1;
                    while (n > 0)
                    {
                        n = sendfile(conn->fd, conn->source_fd, conn->offset, BUF_SIZE);
                    }

                    if (n == -1 && errno == EAGAIN)
                    {
                        printf("Blocked writing to %d\n", conn->fd);
                        continue;
                    }

                    printf("finished sending file to %d\n", conn->fd);
                    close(conn->fd);
                    close(conn->source_fd);
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, conn->fd, NULL);
                    /**
                     * TODO: Free allocated memory
                     * 1) client_conn_data_t
                     * 2) method, uri, version from sscanf
                     */
                }
            }
        }
    }

    return 0;
}