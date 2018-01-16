#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

static char *progname;
static char output[1000];
static void getfds();

int main(int argc, char **argv)
{
    int c, fd = 2, status = 0;
    output[0] = '\0';

    progname = argv[0];
    while ((c = getopt(argc, argv, "W")) != EOF) {
        switch (c) {
        case 'W':
            sprintf(output, "pid %ld run with open fds:", (long)getpid());
            break;
        default:
            status = 1;
        }
    }
    if (status
            || optind < argc - 1
            || (optind == argc - 1 && (fd = atoi(argv[optind])) <= 0)) {
        fprintf(stderr, "usage: %s [-W] [fd]\n", argv[0]);
        return(1);
    }

    getfds();
    strcat(output, "\n");
    write(fd, output, strlen(output));
    return(0);
}


static void getfds()
{
    DIR *dp;
    struct dirent *r;

    if ((dp = opendir("/proc/self/fd")) == NULL) {
        perror("/proc/self/fd");
        fprintf(stderr, "this program only works under linux\n");
        exit(1);
    }

    while ((r = readdir(dp))) {
        if (isdigit(r->d_name[0] & 255)) {
            long x = atoi(r->d_name);
            if (x != dirfd(dp)) {
                if (strlen(output) >= sizeof output - 30) {
                    fprintf(stderr, "%s: string size overflow\n", progname);
                }
                if (output[0])
                    strcat(output, " ");
                sprintf(strchr(output, '\0'), "%ld", x);
            }
        }
    }

    closedir(dp);
}

