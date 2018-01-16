#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

int port = 1234;
char banner[] = "dancecard 1\r\n";
int len;
char msg[500], *tmp;

#define MAXHANDLE 50  /* maximum permitted handle size, not including \0 */

#define FOLLOW 0
#define LEAD 1
#define BOTH 2

struct dancer {
    int fd;
    struct in_addr ipaddr;
    char handle[MAXHANDLE + 1];  /* zero-terminated; handle[0]==0 if not set */
    int role;  /* -1 if not set yet */
    char buf[200];  /* data in progress */
    int bytes_in_buf;  /* how many data bytes already read in buf */
    struct dancer *partner;  /* null if not yet partnered */
    struct dancer *next;
    int partn_s;
} *dancers = NULL;

int nlead = 0, nfollow = 0, nboth = 0, someone_is_partnered = 0;

extern void parseargs(int argc, char **argv);
extern void makelistener(int master_socket, struct sockaddr_in addr);
extern void newclient(int fd, struct sockaddr_in *r, fd_set fds, int max_sd);
extern void begindance();
extern char *memnewline(char *p, int size);  /* finds \r _or_ \n */
extern void do_command(struct dancer *p, char *msg);


int main(int argc, char **argv)
{
    int master_socket = 0, addrlen, max_sd, action;
    int new_socket, len;
    struct sockaddr_in addr;
    fd_set fds; // sockets descriptors
    struct dancer *temp;
    struct dancer *temp1;

    parseargs(argc, argv);

    /* create a master socket */
    if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket");
        return(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    makelistener(master_socket, addr);

    /* accept the incoming connections */
    addrlen = sizeof(addr);

    while (1) {
        temp = dancers;
        /* clear the socket set */
        FD_ZERO(&fds);

        /* add the master socket and children socket to the socket set */
        FD_SET(master_socket, &fds);
        max_sd = master_socket;
        while (temp) {
            if ((*temp).fd > max_sd) {
                max_sd = (*temp).fd;
            }
            FD_SET((*temp).fd, &fds);
            temp = (*temp).next;
        }

        /* wait for clients' action, we wait infinitly */
        action = select(max_sd + 1 , &fds , NULL , NULL , NULL);

        if (action == -1) {
            perror("select");
            return(1);
        }

        else if (action == 0) {
            /* impossible case */
            printf("time out\n");
        }

        else {
            /* it is an incoming connection */
            if (FD_ISSET(master_socket, &fds)) {
                /* we have a new client call accept & newclient() */
                if ((new_socket = accept(master_socket, (struct sockaddr *)&addr, (socklen_t*)&addrlen)) < 0) {
                    perror("accept");
                    return(1);
                }
                newclient(new_socket, &addr, fds, max_sd);
            }

            /* some operation on some old clients */
            temp = dancers;
            while (temp) {
                /* search which client is speaking */
                if (FD_ISSET((*temp).fd, &fds)) {
                    int valread;
                    char buff[1025];
                    valread = read((*temp).fd, buff, 1024);
                    if (valread == 0) {
                        /* we have a disconnection */
                        printf("disconnecting client %s\n", inet_ntoa((*temp).ipaddr));
                        temp1 = dancers;
                        strcat(buff, (*temp).handle);
                        strcat(buff, " bids you all good night.\n");
                        while (temp1) {
                            /* notice all the dancers */
                            if (strcmp((*temp1).handle, (*temp).handle) != 0) {
                                len = strlen(buff);
                                if ((write((*temp1).fd, buff, len)) != len) {
                                    perror("write");
                                    return(1);
                                }
                            }if (!(*temp1).next) {
                                break;
                            }
                            temp1 = (*temp1).next;
                        }
                        memset(buff, 0, strlen(buff));
                        if ((*temp).role == FOLLOW) {
                            nfollow--;
                        }else if ((*temp).role == BOTH) {
                            nboth--;
                        }else if ((*temp).role == LEAD) {
                            nlead--;
                        }
                        temp1 = dancers;
                        if (strcmp((*dancers).handle, (*temp).handle) == 0) {
                            /* the first element */
                            if (!(*dancers).next) {
                                dancers = NULL;
                            }else {
                                dancers = (*dancers).next;
                            }
                        }else {
                            while (temp1) {
                                if ((*temp1).next) {
                                    if (strcmp((*(*temp1).next).handle, (*temp).handle) == 0) {
                                        /* delete it in the linked list */
                                        if ((*(*temp1).next).next) {
                                            (*temp1).next = (*(*temp1).next).next;
                                        }else {
                                            (*temp1).next = NULL;
                                        }
                                    }
                                }
                            }
                        }
                        close((*temp).fd);
                    }else if (valread > 0){
                        /* we have a standard command */
                        do_command(temp, buff);
                    }
                }
                temp = (*temp).next;
            }

        }
    }
}


void parseargs(int argc, char **argv)
{
    int c, status = 0;
    while ((c = getopt(argc, argv, "p:")) != EOF) {
        switch (c) {
        case 'p':
            port = atoi(optarg);
            break;
        default:
            status++;
        }
    }
    if (status || optind != argc) {
        fprintf(stderr, "usage: %s [-p port]\n", argv[0]);
        exit(1);
    }
}


void makelistener(int master_socket, struct sockaddr_in addr)
{
// ... bind and listen ...

    /* bind the socket */
    if (bind(master_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return;
    }

    /* listen 5 max pending connection */
    if (listen(master_socket, 5) < 0) {
        perror("listen");
        return;
    }
}


void newclient(int fd, struct sockaddr_in *r, fd_set fds, int max_sd)
{
    struct dancer *tm1;
    struct dancer *tmp1;
    int role;
    char *p, handle[51], dance_type[10];
    struct dancer *tmp = dancers;

    printf("connection from %s\n", inet_ntoa(r->sin_addr));

    /* write gritting message to client */
    len = strlen(banner);
    if ((write(fd, banner, len)) != len) {
        perror("write");
        return;
    }

    /* get client's handle from client */
    if (read(fd, handle, sizeof(handle)) < 0) {
        perror("read");
        return;
    }

    if ((p = strchr(handle, '\r'))) {
        /* delete the CRLF */
        *p = '\0';
        p[1] = '\0';
    }

    if (strcmp(handle, "who") == 0 || strcmp(handle, "begin") == 0 || strcmp(handle, "debug") == 0) {
        /* handle is one of the command */
        printf("refusing to accept handle '%s'\n", handle);
        len = strlen("Sorry, that word is a command, so it can't be used as a handle.\n");
        write(fd, "Sorry, that word is a command, so it can't be used as a handle.\n", len);
        return;
    }

    while(tmp) {
        /* check for dulipcate handle */
        if (strcmp((*dancers).handle, handle) == 0) {
            printf("refusing to accept handle '%s'\n", handle);
            len = strlen("Sorry, someone is already using that handle.  Please choose another.\n");
            write(fd, "Sorry, someone is already using that handle.  Please choose another.\n", len);
            return;
        }
        tmp = (*tmp).next;
    }

    /* the handle is vaild printf message and call the client */
    printf("set handle of fd %d to %s\n", fd, handle);
    len = strlen("\r\n");
    if ((write(fd, "\r\n", len)) != len) {
        perror("write");
        return;
    }

    /* get the dance type of the client */
    if (read(fd, dance_type, sizeof(dance_type)) < 0) {
        perror("read");
        return;
    }

    /* determind which dance type is */
    if (dance_type[0] == 'l') {
        role = LEAD;
        nlead += 1;
    }else if(dance_type[0] == 'f') {
        role = FOLLOW;
        nfollow += 1;
    }else if(dance_type[0] == 'b') {
        role = BOTH;
        nboth += 1;
    }else {
        /* in vaild dance type */
        len = strlen("Invalid role.  Type lead, follow, or both.\n");
        if ((write(fd, "Invalid role.  Type lead, follow, or both.\n", len)) != len) {
            perror("write");
            return;
        }return;
    }

    /* tell the client everything is set */
    len = strlen("\r\n");
    if ((write(fd, "\r\n", len)) != len) {
        perror("write");
        return;
    }
    printf("set role of fd %d to %d\n", fd, role);


    if (!dancers) {
        dancers = (struct dancer *)malloc(sizeof(struct dancer));
        (*dancers).fd = fd;
        strcpy((*dancers).handle, handle);
        (*dancers).ipaddr = r->sin_addr;
        (*dancers).role = role;
        (*dancers).partner = NULL;
        (*dancers).next = NULL;
        (*dancers).partn_s = 0;
        if (max_sd < fd) {
            max_sd = fd;
        }FD_SET(fd, &fds);
        len = strlen("No one else is here yet, but I'm sure they'll be here soon!\n");
        write(fd, "No one else is here yet, but I'm sure they'll be here soon!\n", len);
        return;
    }else {
        char mesg[100];
        len = strlen("Unpartnered dancers are:\n");
        write(fd, "Unpartnered dancers are:\n", len);
        strcat(mesg, handle);
        strcat(mesg, " has joined the dance!\n");
        len = strlen(mesg);
        tmp1 = dancers;
        while(tmp1) {
            /* notice all other clients */
            strcat(mesg, handle);
            strcat(mesg, " has joined the dance!\n");
            if ((write((*tmp1).fd, mesg, len)) != len) {
                perror("write");
                return;
            }
            memset(mesg, 0, strlen(mesg));
            strcat(mesg, (*tmp1).handle);
            strcat(mesg, "\n");
            write(fd, mesg, strlen(mesg));
            memset(mesg, 0, strlen(mesg));
            if ((*tmp1).next == NULL) {
                tm1 = (struct dancer *)malloc(sizeof(struct dancer));
                (*tm1).fd = fd;
                strcpy((*tm1).handle, handle);
                (*tm1).ipaddr = r->sin_addr;
                (*tm1).role = role;
                (*tm1).partner = NULL;
                (*tm1).next = NULL;
                (*tm1).partn_s = 0;
                (*tmp1).next = tm1;
                if (max_sd < fd) {
                    max_sd = fd;
                }FD_SET(fd, &fds);
                return;
            }memset(mesg, 0, strlen(mesg));
            strcat(mesg, (*tmp1).handle);
            strcat(mesg, "\n");
            write(fd, mesg, strlen(mesg));
            memset(mesg, 0, strlen(mesg));
            tmp1 = (*tmp1).next;
        }
    }
}

void do_command(struct dancer *p, char *msg)
{

    if ((tmp = strchr(msg, '\r')) != NULL) {
        /* delete the CRLF */
        *tmp = '\0';
        tmp[1] = '\0';
    }

    /* determind what command the client enter */
    if (strcmp(msg, "who") == 0) {
        if (nboth + nlead + nfollow == 1) {
            /* you are the only dancer */
            len = strlen("No one else is here yet, but I'm sure they'll be here soon!\r\n");
            write((*p).fd, "No one else is here yet, but I'm sure they'll be here soon!\r\n", len);
        }
        else {
            /* you are alone~ */
            struct dancer *temp = dancers;
            memset(msg, 0, strlen(msg));
            strcat(msg, "Unpartnered dancers are:\n");
            while (temp) {
                if (strcmp((*temp).handle, (*p).handle) != 0 && (*temp).partner == NULL) {
                    if ((*temp).role == (*p).role && (*temp).role != BOTH) {
                        strcat(msg, "[");
                        strcat(msg, (*temp).handle);
                        strcat(msg, " only dances ");
                        if ((*temp).role == FOLLOW) {
                            strcat(msg, "follow]\n");
                        }else {
                            strcat(msg, "lead]\n");
                        }
                    }else {
                        strcat(msg, (*temp).handle);
                        strcat(msg, "\n");
                    }
                }
                temp = (*temp).next;
            }
            len = strlen(msg);
            if ((write((*p).fd, msg, len)) != len) {
                perror("write");
                return;
            }
        }return;
    }else if (strcmp(msg, "debug") == 0) {
        char numtp[12];
        memset(msg, 0, strlen(msg));
        strcat(msg, "nlead ");
        sprintf(numtp, "%d", nlead);
        strcat(msg, numtp);
        memset(numtp, 0, strlen(numtp));
        strcat(msg, ", nfollow ");
        sprintf(numtp, "%d", nfollow);
        strcat(msg, numtp);
        memset(numtp, 0, strlen(numtp));
        strcat(msg, ", nboth ");
        sprintf(numtp, "%d", nboth);
        strcat(msg, numtp);
        strcat(msg, "\n");
        memset(numtp, 0, strlen(numtp));
        len = strlen(msg);
        if ((write((*p).fd, msg, len)) != len) {
            perror("write");
            return;
        }return;
    }else if (strcmp(msg, "begin") == 0) {
        /* dance begins anyway */
        begindance();
        return;
    }else {
        /* it must be an invitation */
        if (strcmp(msg, (*p).handle) == 0) {
            /* you can not dance with yourself */
            len = strlen("This is a couples dance -- you can't dance with yourself.\n");
            if ((write((*p).fd, "This is a couples dance -- you can't dance with yourself.\n", len)) != len) {
                perror("write");
                return;
            }
        }else {
            struct dancer *temp = dancers;
            temp = dancers;
            while (temp) {
                /* search the client */
                if (strcmp((*temp).handle, msg) == 0) {
                    if ((*temp).partner) {
                        /* this client has a partner */
                        memset(msg, 0, strlen(msg));
                        strcat(msg, (*temp).handle);
                        strcat(msg, " already has a partner for this dance.  Try again next dance!\n");
                        len = strlen(msg);
                        if ((write((*p).fd, msg, len)) != len) {
                            perror("write");
                            return;
                        }memset(msg, 0, strlen(msg));
                    }else if ((*p).role != BOTH && (*temp).role == (*p).role) {
                        /* you two dance the same role */
                        memset(msg, 0, strlen(msg));
                        strcat(msg, "Sorry, ");
                        strcat(msg, (*temp).handle);
                        strcat(msg, " can only dance ");
                        if ((*p).role == LEAD) {
                            strcat(msg, "lead.\n");
                        }else {
                            strcat(msg, "follow.\n");
                        }len = strlen(msg);
                        if ((write((*p).fd, msg, len)) != len) {
                            perror("write");
                            return;
                        }memset(msg, 0, strlen(msg));
                    }else {
                        if ((((*p).role == FOLLOW || (*p).role == BOTH) && nfollow < nlead && ((*temp).role == BOTH || (*temp).role == FOLLOW)) 
                            || (((*p).role == LEAD || (*p).role == BOTH) && nfollow > nlead && ((*temp).role == BOTH || (*temp).role == LEAD))) {
                            /* this client is making the pair worse */
                            memset(msg, 0, strlen(msg));
                            strcat(msg, "Please don't ask ");
                            strcat(msg, (*temp).handle);
                            strcat(msg, " to dance, as that would leave people out.\n");
                            len = strlen(msg);
                            if ((write((*p).fd, msg, len)) != len) {
                                perror("write");
                                return;
                            }memset(msg, 0, strlen(msg));
                        }else {
                            /* now we are talking */
                            memset(msg, 0, strlen(msg));
                            strcat(msg, (*temp).handle);
                            strcat(msg, " accepts!\n");
                            len = strlen(msg);
                            if ((write((*p).fd, msg, len)) != len) {
                                perror("write");
                                return;
                            }memset(msg, 0, strlen(msg));
                            strcat(msg, (*p).handle);
                            strcat(msg, "has asked you to dance.  You accept!\n");
                            len = strlen(msg);
                            if ((write((*temp).fd, msg, len)) != len) {
                                perror("write");
                                return;
                            }memset(msg, 0, strlen(msg));
                            (*p).partner = temp;
                            (*p).partn_s = 1;
                            (*temp).partner = p;
                            (*temp).partn_s = 1;
                            /* update the data */
                            if ((*p).role == BOTH) {
                                nboth--;
                            }else if ((*p).role == LEAD) {
                                nlead--;
                            }else if ((*p).role == FOLLOW) {
                                nfollow--;
                            }if ((*temp).role == BOTH) {
                                nboth--;
                            }else if ((*temp).role == FOLLOW) {
                                nfollow--;
                            }else if ((*temp).role == LEAD) {
                                nlead--;
                            }
                            someone_is_partnered += 2;
                            if ((nboth == 0 && nlead == 0) || (nboth == 0 && nfollow == 0)) {
                                /* check if they are already for the dance */
                                begindance();
                                return;
                            }
                        }
                    }
                }temp = (*temp).next;
            }
        }
    }len = strlen("no one is by that name, please try 'who' to see the list of dancers\n");
    write((*p).fd, "no one is by that name, please try 'who' to see the list of dancers\n", len);
    return;
}


void begindance()
{
    static char message1[] = "Dance begins\r\n";
    static char message2[] = "Dance ends.  Your partner thanks you for the dance!\r\nTime to find a new partner.  Type 'who' for a list of available dancers.\r\n";
    struct dancer *p;
    for (p = dancers; p; p = p->next)
        write(p->fd, message1, sizeof message1 - 1);
    sleep(5);
    for (p = dancers; p; p = p->next) {
        write(p->fd, message2, sizeof message2 - 1);
        p->partner = NULL;
    }
    someone_is_partnered = 0;
    // something to reset nlead, etc --
    //   I suggest counting again from scratch, probably as a separate function
    nlead = nfollow = nboth = 0;
    p = dancers;
    while (p) {
        (*p).partner = NULL;
        (*p).partn_s = 0;
        if ((*p).role == BOTH) {
            nboth++;
        }else if((*p).role == FOLLOW) {
            nfollow++;
        }else if ((*p).role == LEAD) {
            nlead++;
        }
        p = (*p).next;
    }
}


char *memnewline(char *p, int size)  /* finds \r _or_ \n */
        /* This is like min(memchr(p, '\r'), memchr(p, '\n')) */
        /* It is named after memchr().  There's no memcspn(). */
{
    for (; size > 0; p++, size--)
        if (*p == '\r' || *p == '\n')
            return(p);
    return(NULL);
}

