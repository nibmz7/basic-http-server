#include <limits.h>
#include <stdint.h> // int64_t
#include <sys/types.h>

#define BUF_SIZE 4096 * 1000
#define RES_HEADER_SIZE 32000

typedef struct
{
    char *port;
    char *dir;
} option_t;

typedef struct
{
    int fd;
    int64_t size;
    char *mime_type;
} file_info_t;

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

char cwd[PATH_MAX];

void setcwd(char *path);
void resolve(char *uri, char **result);

int get_static_file_info(char *uri, file_info_t *info);
void get_options(int argc, char **args, option_t *option);

int create_listening_socket(char *port, int epollfd);
void accept_connections(int listen_sd, int epollfd);

void client_read(client_conn_data_t *conn, int epollfd);
void client_write(client_conn_data_t *conn, int epollfd);
void client_close(client_conn_data_t *conn, int epollfd);
