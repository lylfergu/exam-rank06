#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int maxfd, next_id;
int ids[65536];
char *bufs[65536];
fd_set all;

void fatal()
{
    write(2, "Fatal error\n", 12);
    exit(1);
}

void send_all(int except, char *msg)
{
    for (int fd = 0; fd <= maxfd; fd++)
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

int extract_msg(char **buf, char **msg)
{
    char *newbuf;
    int i = 0;

    *msg = 0;
    if (!buf)
        return (0);

    while((*buf)[i])
    {
        if ((*buf)[i] == '\n')
        {
            newbuf = calloc(0, strlen(*buf + i + 1) + 1);
            if (!newbuf)
                return (-1);
            strcpy(newbuf, *buf +i + 1);
            *msg = *buf;
            (*msg)[i + 1] = 0;
            *buf = newbuf;
            return (1);
        }
        i++;
    }
    return (0);
}

int main(int arc, char **argv)
{
    int sock, fd, n;
    struct sockaddr_in readfds;
    char *buf[1001];
    char *msg[100000];
    fd_set addr;

    if (arc != 2)
    {
        write(2, "Wrong number of arguments\n", 26);
        exit (1);
    }
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        fatal();

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = hton(21307)