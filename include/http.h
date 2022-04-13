#include <sys/types.h>
#include <unistd.h>

typedef struct
{
    char *name, *value;
} http_request_header_t;

typedef struct
{
    const char *method;
    const char *uri;
} http_request_info_t;

typedef struct
{
    const http_request_info_t *info;
    const http_request_header_t *headers;
    const int headers_size;
} http_request_t;

typedef struct
{
    const http_request_t *request;
} http_conn_context_t;

typedef void (*http_on_client_request_t)(const http_conn_context_t *http_conn_context);

void http_start(const http_on_client_request_t *on_client_request, int port);