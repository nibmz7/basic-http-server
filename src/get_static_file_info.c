#include "http.h"
#include <sys/stat.h> // stat
#include <stdlib.h>   // exit
#include <fcntl.h>    // open
#include <stdio.h>    // perror
#include <string.h>   // strlen, memcmp

int strEndsWith(const char *s, const char *suff)
{
    size_t slen = strlen(s);
    size_t sufflen = strlen(suff);

    return slen >= sufflen && !memcmp(s + slen - sufflen, suff, sufflen);
}

int get_static_file_info(char *uri, file_info_t *info)
{
    char *file_path;
    resolve(uri, &file_path);

    struct stat stat_buf;
    if (stat(file_path, &stat_buf) == -1)
    {
        printf("Error: %s\n", uri);
        perror("stat");
        return -1;
    }

    info->size = stat_buf.st_size;

    // ulimit -ah shows max no.of open file descriptors for a process ->  1048576
    int fd = open(file_path, O_RDONLY);

    info->fd = fd;

    if (strEndsWith(file_path, ".js"))
    {
        info->mime_type = "text/javascript";
    }
    else
    {
        info->mime_type = "text/html";
    }

    return 0;
}