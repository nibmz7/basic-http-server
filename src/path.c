#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>  // perror
#include <stdlib.h> // exit, calloc

char cwd[PATH_MAX] = {'h', '\0'};

/**
 * Handle ERANGE error
 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/getcwd.html
 */
void setcwd(char *path)
{
    int path_size = strlen(path);
    if (*path == '/')
    {
        realpath(path, cwd);
    }
    else
    {
        char current_cwd[PATH_MAX];
        getcwd(current_cwd, PATH_MAX);
        int current_cwd_size = strlen(current_cwd);
        char fullpath[PATH_MAX] = {'\0'};
        strncat(fullpath, current_cwd, current_cwd_size);
        strncat(fullpath, "/", 1);
        strncat(fullpath, path, path_size);
        realpath(fullpath, cwd);
    }
}

void resolve(char *uri, char **result)
{
    int uri_size = strlen(uri);
    char buf[PATH_MAX] = {'\0'};
    char file_path[PATH_MAX] = {'\0'};

    strncat(file_path, cwd, strlen(cwd));
    strncat(file_path, uri, uri_size);
    realpath(file_path, buf);

    if (uri[uri_size - 1] == '/')
    {
        strncat(buf, "/index.html", 11);
    }

    *result = buf;
}
