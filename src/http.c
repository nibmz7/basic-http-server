#include <stdlib.h>
#include<stdio.h>
#include <sys/epoll.h>    // for epoll_create1(), epoll_ctl(), struct epoll_event
#include "http.h"

#define MAX_EVENTS 10

int main(int argc, char *argv[])
{
    option_t option;

    get_options(argc, argv, &option);
    setcwd(option.dir);
    printf("port: %s\ncwd: %s\n", option.port, cwd);

    struct epoll_event ev;
    int nfds, epollfd, n;

    // Create epoll file descriptor
    epollfd = epoll_create1(0);
    if (epollfd == -1)
    {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    int listen_sd = create_listening_socket(option.port, epollfd);

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
                accept_connections(listen_sd, epollfd);
            }
            else
            {
                // Read/write from/to client socket
                client_conn_data_t *conn = events[n].data.ptr;
                if (((events[n].events & EPOLLIN)))
                {
                    client_read(conn, epollfd);
                }
                else if (((events[n].events & EPOLLOUT)))
                {
                    client_write(conn, epollfd);
                }
            }
        }
    }

    return 0;
}