#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>

int maxfd, next_id;
int ids[65536];
char *buf[65536];
fd_set all;

void fatal(void)
{
    write(2, "Fatal error\n", 12);
    exit(1);
}

void send_all(int except, char *msg)
{
    for (int fd = 0; fd <= maxfd; i++)
    {
        if (ids[fd] != -1 && fd != except)
            send(fd, msg, strlen(msg), 0);
    }
}

char *strjoin(char *buf, char *add)
{
    char *newbuf;
    int len = buf ? strlen(buf) : 0;

    newbuf = malloc(len + strlen(add) + 1);
    if (!newbuf)
        return (0);

    newbuf[0] = 0;

    if (buf)
        strcat(newbuf, buf);
    strcat(newbuf, add);
    free(buf);
    return (newbuf);
}

int extract_message(char **buf, char **msg)
{
    char *newbuf;
    int i = 0;

    
}