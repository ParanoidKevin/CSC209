#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define MAXHANDLE 50

int serverfd;

char myhandle[MAXHANDLE + 1];
int handle_on_side = 0;

/* data from server */
char buf[500];
int bytes_in_buf = 0;
char *nextpos;

extern void process_data_from_server();
extern char *read_from_server();  /* read from server, remove network newline */
extern char *extractline(char *p, int size);
extern char *memnewline(char *p, int size);  /* finds \r _or_ \n */


int main(int argc, char **argv)
{
    int c, status = 0;
    int port = 1234;  /* default port number */
    int len;
    char msg[500], *p;
    extern void interact(char *prompt, char *storeit);
    extern void connect_to_server(char *host, int port);

    while ((c = getopt(argc, argv, "h")) != EOF) {
        switch (c) {
        case 'h':
            handle_on_side = 1;
            break;
        default:
            status = 1;
        }
    }

    if (!status && argc - optind == 2) {
        if ((port = atoi(argv[optind+1])) == 0) {
            fprintf(stderr, "%s: port number must be a positive integer\n", argv[0]);
            return(1);
        }
    } else if (status || argc - optind != 1) {
        fprintf(stderr, "usage: %s [-h] host [port]\n", argv[0]);
        return(1);
    }

    connect_to_server(argv[optind], port);

    interact("Enter your 'handle':", myhandle);
    interact("Can you dance lead, follow, or both?", NULL);

    while (1) {
        if (handle_on_side) {
            printf("%s| ", myhandle);
            fflush(stdout);
        }
        if (memnewline(nextpos, bytes_in_buf)) {
            process_data_from_server();
        } else {
            fd_set fdlist;
            FD_ZERO(&fdlist);
            FD_SET(serverfd, &fdlist);
            FD_SET(0, &fdlist);
            if (select(serverfd + 1, &fdlist, NULL, NULL, NULL) < 0) {
                perror("select");
                exit(1);
            }
            if (FD_ISSET(serverfd, &fdlist)) {
                /* something happen on the server side */
                process_data_from_server();
            } else if (fgets(msg, sizeof msg - 2, stdin) == NULL) {
                exit(0);
            } else {
                if ((p = strchr(msg, '\n')))
                    *p = '\0';
                strcat(msg, "\r\n");
                len = strlen(msg);
                if (write(serverfd, msg, len) != len)
                    perror("write");
            }
        }
    }
}


void interact(char *prompt, char *storeit)
{
    char msg[500], *p;
    int len;
    while (1) {
        printf("%s ", prompt);
        if (fgets(msg, sizeof msg, stdin) == NULL)
            exit(0);
        if ((p = strchr(msg, '\n')))
            *p = '\0';
        if (!msg[0])
            continue;
        msg[MAXHANDLE] = '\0';  /* if longer than that, truncate it */
        if (storeit)
            strcpy(storeit, msg);  /* storeit has to be size >= MAXHANDLE+1 */
        strcat(msg, "\r\n");
        len = strlen(msg);
        if (write(serverfd, msg, len) != len) {
            perror("write");
            exit(1);
        }
        while (!(p = read_from_server()))
            ;
        if (p[0])
            printf("%s\n", p);
        else
            return;
    }
}


void connect_to_server(char *host, int port)
{
    struct hostent *hp;
    struct sockaddr_in r;
    char *p;

    if ((hp = gethostbyname(host)) == NULL) {
        fprintf(stderr, "%s: no such host\n", host);
        exit(1);
    }
    if (hp->h_addr_list[0] == NULL || hp->h_addrtype != AF_INET) {
        fprintf(stderr, "%s: not an internet protocol host name\n", host);
        exit(1);
    }

    if ((serverfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    memset(&r, '\0', sizeof r);
    r.sin_family = AF_INET;
    memcpy(&r.sin_addr, hp->h_addr_list[0], hp->h_length); /* changed */
    r.sin_port = htons(port); /* extra */

    if (connect(serverfd, (struct sockaddr *)&r, sizeof r) < 0) {
        perror("connect");
        exit(1);
    }

    while ((p = read_from_server()) == NULL)
        ;
    if (strcmp(p, "dancecard 1")) {
        fprintf(stderr, "That is not a Dance Card server.\n");
        exit(1);
    }
}


void process_data_from_server()
{
    char *p;
    if ((p = read_from_server()))
        printf("%s\n", p);
}


char *read_from_server()
{
    int nbytes;

    /* move the leftover data to the beginning of buf */
    if (bytes_in_buf && nextpos)
        memmove(buf, nextpos, bytes_in_buf);

    /* If we've already got another whole line, return it without a read() */
    if ((nextpos = extractline(buf, bytes_in_buf))) {
        bytes_in_buf -= (nextpos - buf);
        return(buf);
    }

    /*
     * Ok, try a read().  Note that we _never_ fill the buffer, so that there's
     * always room for a \0.
     */
    nbytes = read(serverfd, buf + bytes_in_buf, sizeof buf - bytes_in_buf - 1);
    if (nbytes <= 0) {
        if (nbytes < 0)
            perror("read()");
        printf("Server shut down\n");
        exit(0);
    }

    bytes_in_buf += nbytes;

    /* So, _now_ do we have a whole line? */
    if ((nextpos = extractline(buf, bytes_in_buf))) {
        bytes_in_buf -= (nextpos - buf);
        return(buf);
    }
    /*
     * Don't do another read(), to avoid the possibility of blocking.
     * However, if we've hit the maximum message size, we should call
     * it all a line.
     */
    if (bytes_in_buf == sizeof buf - 1) {
        buf[bytes_in_buf] = '\0';
        bytes_in_buf = 0;
        nextpos = NULL;
        return(buf);
    }

    /* If we got to here, we don't have a full input line yet. */
    return(NULL);
}


char *extractline(char *p, int size)
        /* returns pointer to string after, or NULL if there isn't an entire
         * line here */
{
    char *nl = memnewline(p, size);
    if (!nl)
        return(NULL);

    /*
     * There are three cases: either this is a lone \r, a lone \n, or a CRLF.
     */
    if (*nl == '\r' && nl - p < size && nl[1] == '\n') {
        /* CRLF */
        *nl = '\0';
        return(nl + 2);
    } else {
        /* lone \n or \r */
        *nl = '\0';
        return(nl + 1);
    }
}


char *memnewline(char *p, int size)  /* finds \r _or_ \n */
        /* This is like min(memchr(p, '\r'), memchr(p, '\n')) */
        /* It is named after memchr().  There's no memcspn(). */
{
    while (size > 0) {
        if (*p == '\r' || *p == '\n')
            return(p);
        p++;
        size--;
    }
    return(NULL);
}

