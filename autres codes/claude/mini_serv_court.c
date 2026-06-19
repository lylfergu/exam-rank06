#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int		max_fd, next_id;
int		ids[65536];
char	*bufs[65536];
fd_set	fds;

void	fatal() { write(2, "Fatal error\n", 12); exit(1); }

void	send_all(int except, char *msg)
{
	for (int fd = 0; fd <= max_fd; fd++)
		if (ids[fd] >= 0 && fd != except)
			send(fd, msg, strlen(msg), 0);
}

void	on_connect(int srv)
{
	char msg[64];
	int fd = accept(srv, 0, 0);
	if (fd < 0) return;
	ids[fd] = next_id++;
	bufs[fd] = NULL;
	FD_SET(fd, &fds);
	if (fd > max_fd) max_fd = fd;
	sprintf(msg, "server: client %d just arrived\n", ids[fd]);
	send_all(fd, msg);
}

void	on_disconnect(int fd)
{
	char msg[64];
	sprintf(msg, "server: client %d just left\n", ids[fd]);
	send_all(fd, msg);
	FD_CLR(fd, &fds);
	free(bufs[fd]);
	bufs[fd] = NULL;
	ids[fd] = -1;
	close(fd);
}

void	on_message(int fd)
{
	char tmp[4097];
	int  r = recv(fd, tmp, 4096, 0);
	if (r <= 0) { on_disconnect(fd); return; }
	tmp[r] = '\0';
	int old = bufs[fd] ? strlen(bufs[fd]) : 0;
	if (!(bufs[fd] = realloc(bufs[fd], old + r + 1))) fatal();
	if (!old) bufs[fd][0] = '\0';
	strcat(bufs[fd], tmp);
	char *s = bufs[fd], *nl, msg[5000];
	while ((nl = strstr(s, "\n")))
	{
		*nl = '\0';
		sprintf(msg, "client %d: %s\n", ids[fd], s);
		send_all(fd, msg);
		s = nl + 1;
	}
	int left = strlen(s);
	if (!left) { free(bufs[fd]); bufs[fd] = NULL; }
	else { strcpy(bufs[fd], s); if (!(bufs[fd] = realloc(bufs[fd], left + 1))) fatal(); }
}

int	main(int argc, char **argv)
{
	if (argc != 2) { write(2, "Wrong number of arguments\n", 26); exit(1); }
	int srv = socket(AF_INET, SOCK_STREAM, 0);
	if (srv < 0) fatal();
	struct sockaddr_in addr = {AF_INET, htons(atoi(argv[1])), {inet_addr("127.0.0.1")}};
	if (bind(srv, (struct sockaddr *)&addr, sizeof(addr)) < 0) fatal();
	if (listen(srv, 128) < 0) fatal();
	memset(ids, -1, sizeof(ids));
	FD_ZERO(&fds);
	FD_SET(srv, &fds);
	max_fd = srv;
	while (1)
	{
		fd_set tmp = fds;
		if (select(max_fd + 1, &tmp, 0, 0, 0) < 0) continue;
		for (int fd = 0; fd <= max_fd; fd++)
			if (FD_ISSET(fd, &tmp))
				fd == srv ? on_connect(srv) : on_message(fd);
	}
}