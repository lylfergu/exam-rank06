//version apporuvee par claude et chat


#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

int		maxfd, next_id;
int		ids[65536];
char	*bufs[65536];
fd_set	all;

void	fatal(void)
{
	write(2, "Fatal error\n", 12);
	exit(1);
}

void	send_all(int except, char *msg)
{
	for (int fd = 0; fd <= maxfd; fd++)
	{
		if (ids[fd] != -1 && fd != except)
			send(fd, msg, strlen(msg), 0);
	}
}

char	*str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len = buf ? strlen(buf) : 0;

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


int	extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int		i = 0;

	*msg = 0;
	if (!*buf)
		return (0);
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, strlen(*buf + i + 1) + 1);
			if (!newbuf)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}


int	main(int argc, char **argv)
{
	int					sock, fd, n;
	struct sockaddr_in	addr;
	fd_set				readfds;
	char				buf[1001];
	char				msg[100000];

	if (argc != 2)
	{
		write(2, "Wrong number of arguments\n", 26);
		return (1);
	}

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		fatal();

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(2130706433);
	addr.sin_port = htons(atoi(argv[1]));

	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		fatal();
	if (listen(sock, 128) < 0)
		fatal();

	for (int i = 0; i < 65536; i++)
		ids[i] = -1;

	FD_ZERO(&all);
	FD_SET(sock, &all);
	maxfd = sock;

	while (1)
	{
		readfds = all;
		if (select(maxfd + 1, &readfds, 0, 0, 0) < 0)
			continue;

		for (fd = 0; fd <= maxfd; fd++)
		{
			if (!FD_ISSET(fd, &readfds))
				continue;

			if (fd == sock)
			{
				int client = accept(sock, 0, 0);
				if (client < 0)
					continue;

				if (client > maxfd)
					maxfd = client;

				ids[client] = next_id++;
				bufs[client] = NULL;
				FD_SET(client, &all);

				sprintf(msg,
					"server: client %d just arrived\n",
					ids[client]);
				send_all(client, msg);
			}
			else
			{
				n = recv(fd, buf, 1000, 0);

				if (n <= 0)
				{
					sprintf(msg,
						"server: client %d just left\n",
						ids[fd]);
					send_all(fd, msg);

					free(bufs[fd]);
					bufs[fd] = NULL;
					ids[fd] = -1;

					FD_CLR(fd, &all);
					close(fd);
				}
				else
				{
					char	*line;

					buf[n] = 0;
					bufs[fd] = str_join(bufs[fd], buf);
					if (!bufs[fd])
						fatal();

					while (extract_message(&bufs[fd], &line) == 1)
					{
						sprintf(msg, "client %d: %s", ids[fd],line);
						send_all(fd, msg);
						free(line);
					}
					if (bufs[fd] && bufs[fd][0] == '\0')
					{
						free(bufs[fd]);
						bufs[fd] = NULL;
					}
				}
			}
		}
	}
}