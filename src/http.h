#include <limits.h>
#include <stdint.h> // int64_t

typedef struct
{
    int port;
    char *dir;
} option_t;

typedef struct
{
    int fd;
    int64_t size;
    char *mime_type;
} file_info_t;

char cwd[PATH_MAX];

void setcwd(char *path);
void resolve(char *uri, char **result);

int get_static_file_info(char *uri, file_info_t *info);
void get_options(int argc, char **args, option_t *option);