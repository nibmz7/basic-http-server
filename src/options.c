#include <getopt.h> // getopt_long_only
#include <string.h> // strcmp
#include <unistd.h> // optarg
#include <stdlib.h> // atoi
#include "http.h"

void get_options(int argc, char **args, option_t *option)
{
    char *port = "80";
    char *dir = "./";

    static struct option long_options[] = {
        {"port", required_argument, 0, 0},
        {"dir", required_argument, 0, 0},
        {0, 0, 0, 0}};

    int c;
    int option_index = 0;

    while (1)
    {

        c = getopt_long_only(argc, args, "",
                             long_options, &option_index);

        if (c == -1)
        {
            break;
        }

        if (optarg)
        {
            const char *option_name = long_options[option_index].name;

            if (strcmp(option_name, "port") == 0)
            {
                port = optarg;
            }
            else if (strcmp(option_name, "dir") == 0)
            {
                dir = optarg;
            }
        }
    }

    option->port = port;
    option->dir = dir;
}