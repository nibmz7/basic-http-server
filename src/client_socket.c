#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/epoll.h>    // for epoll_create1(), epoll_ctl(), struct epoll_event
#include <sys/sendfile.h> // for sendfile()
#include <stdlib.h>       // EXIT_FAILURE
#include "http.h"

void client_read(client_conn_data_t *conn, int epollfd)
{
    struct epoll_event ev;
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
                exit(EXIT_FAILURE);
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
                        ev.events = EPOLLOUT | EPOLLET;
                        ev.data.ptr = conn;
                        epoll_ctl(epollfd, EPOLL_CTL_MOD, conn->fd, &ev);
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

void client_write(client_conn_data_t *conn, int epollfd)
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
        return;
    }

    printf("finished sending file to %d\n", conn->fd);

    client_close(conn, epollfd);
}

void client_close(client_conn_data_t *conn, int epollfd)
{
    printf("Closed client %d\n", conn->fd);
    epoll_ctl(epollfd, EPOLL_CTL_DEL, conn->fd, NULL);
    close(conn->fd);
    close(conn->source_fd);
    free(conn->method);
    free(conn->uri);
    free(conn->version);
    free(conn);
}
