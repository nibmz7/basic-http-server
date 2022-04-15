// #include <stdio.h>
// #include <sys/socket.h>
// #include <unistd.h>
// #include <stdlib.h>
// #include <netinet/in.h>
// #include <string.h>
// #include <getopt.h>
// #include <sys/stat.h>
// #include <fcntl.h>
// #include <sys/types.h>
// #include <sys/uio.h>

// #define RES_HEADER_SIZE 32000

// int main(int argc, char *argv[])
// {
//     int server_fd, new_socket;
//     struct sockaddr_in address;
//     int addrlen = sizeof(address);

//     int port = 8000;
//     const char *dir = "./";

//     static struct option long_options[] = {
//         {"port", required_argument, 0, 0},
//         {"dir", required_argument, 0, 0},
//         {0, 0, 0, 0}};

//     int c;
//     int option_index = 0;

//     while (1)
//     {

//         c = getopt_long_only(argc, argv, "",
//                              long_options, &option_index);
//         if (c == -1)
//         {
//             break;
//         }

//         if (optarg)
//         {
//             const char *option_name = long_options[option_index].name;

//             if (strcmp(option_name, "port") == 0)
//             {
//                 port = atoi(optarg);
//             }
//             else if (strcmp(option_name, "dir") == 0)
//             {
//                 dir = optarg;
//             }
//         }
//     }

//     printf("%d %s\n", port, dir);

//     // Creating socket file descriptor
//     if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
//     {
//         perror("In socket");
//         exit(EXIT_FAILURE);
//     }

//     /**
//      * https://www.ibm.com/docs/ja/zvm/7.2?topic=domains-network-byte-order-host-byte-order
//      * htonl converts a long integer (e.g. address) to a network representation
//      * htons converts a short integer (e.g. port) to a network representation
//      */
//     address.sin_family = AF_INET;
//     address.sin_addr.s_addr = INADDR_ANY;
//     address.sin_port = htons(port);

//     memset(address.sin_zero, '\0', sizeof address.sin_zero);

//     if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
//     {
//         perror("In bind");
//         exit(EXIT_FAILURE);
//     }

//     if (listen(server_fd, 10) < 0)
//     {
//         perror("In listen");
//         exit(EXIT_FAILURE);
//     }

//     while (1)
//     {
//         printf("\n+++++++ Waiting for new connection ++++++++\n\n");
//         if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
//         {
//             perror("In accept");
//             exit(EXIT_FAILURE);
//         }
//         printf("------------------New connection accepted-------------------\n\n");

//         int offset;
//         char res_header_buf[RES_HEADER_SIZE];
//         struct stat stat_buf;
//         int fromfd = open("./website/index.html", O_RDONLY);
//         if (fstat(fromfd, &stat_buf) == -1)
//         {
//             perror("lstat");
//             exit(EXIT_FAILURE);
//         }
//         int n = 1;
//         off_t BUF_SIZE = stat_buf.st_size;
//         offset = snprintf(res_header_buf, RES_HEADER_SIZE, "HTTP/1.1 200 OK\r\n");
//         offset += snprintf(res_header_buf + offset, RES_HEADER_SIZE, "Server: Basic Web Server\r\n");
//         offset += snprintf(res_header_buf + offset, RES_HEADER_SIZE, "Content-length: %d\r\n", (int)stat_buf.st_size);
//         offset += snprintf(res_header_buf + offset, RES_HEADER_SIZE, "Content-type: %s\r\n", "text/html");
//         offset += snprintf(res_header_buf + offset, RES_HEADER_SIZE, "\r\n");
//         write(new_socket, res_header_buf, offset);

//         while (n > 0)
//         {
//             // osx sendfile https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man2/sendfile.2.html
//             n = sendfile(fromfd, new_socket, 0, &BUF_SIZE, NULL, 0);
//         }
//         if (n == -1)
//         {
//             perror("sendfile");
//             exit(EXIT_FAILURE);
//         }
//         printf("------------------Response sent-------------------\n");
//         close(new_socket);
//     }
//     return 0;
// }