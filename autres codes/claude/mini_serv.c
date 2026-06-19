#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct s_client
{
	int		fd;
	int		id;
	char	*buf;
} t_client;

static t_client	clients[65536];
static int		next_id = 0;
static int		max_fd = 0;
static fd_set	active_fds;

static void	fatal(void)
{
	write(2, "Fatal error\n", 12);
	exit(1);
}

static void	send_to_all_except(int except_fd, char *msg)
{
	for (int fd = 0; fd <= max_fd; fd++)
	{
		if (clients[fd].fd > 0 && fd != except_fd)
			send(fd, msg, strlen(msg), 0);
	}
}

static void	client_connect(int server_fd)
{
	struct sockaddr_in	addr;
	socklen_t			len = sizeof(addr);
	int					fd;
	char				msg[64];

	fd = accept(server_fd, (struct sockaddr *)&addr, &len);
	if (fd < 0)
		return ;
	clients[fd].fd = fd;
	clients[fd].id = next_id++;
	clients[fd].buf = NULL;
	FD_SET(fd, &active_fds);
	if (fd > max_fd)
		max_fd = fd;
	sprintf(msg, "server: client %d just arrived\n", clients[fd].id);
	send_to_all_except(fd, msg);
}

static void	client_disconnect(int fd)
{
	char	msg[64];

	sprintf(msg, "server: client %d just left\n", clients[fd].id);
	send_to_all_except(fd, msg);
	FD_CLR(fd, &active_fds);
	free(clients[fd].buf);
	clients[fd].buf = NULL;
	clients[fd].fd = 0;
	close(fd);
}

static void	process_message(int fd)
{
	char	tmp[4096];
	int		ret;

	ret = recv(fd, tmp, sizeof(tmp) - 1, 0);
	if (ret <= 0)
	{
		client_disconnect(fd);
		return ;
	}
	tmp[ret] = '\0';

	// Append to client buffer
	int		old_len = clients[fd].buf ? strlen(clients[fd].buf) : 0;
	char	*new_buf = realloc(clients[fd].buf, old_len + ret + 1);
	if (!new_buf)
		fatal();
	clients[fd].buf = new_buf;
	memcpy(clients[fd].buf + old_len, tmp, ret + 1);

	// Process complete lines
	char	*start = clients[fd].buf;
	char	*nl;
	while ((nl = strstr(start, "\n")) != NULL)
	{
		*nl = '\0';
		// Build "client X: line\n" and send to all others
		char	header[32];
		sprintf(header, "client %d: ", clients[fd].id);
		int		hlen = strlen(header);
		int		llen = strlen(start);
		char	*msg = malloc(hlen + llen + 2);
		if (!msg)
			fatal();
		strcpy(msg, header);
		strcpy(msg + hlen, start);
		msg[hlen + llen] = '\n';
		msg[hlen + llen + 1] = '\0';
		send_to_all_except(fd, msg);
		free(msg);
		start = nl + 1;
	}

	// Keep leftover (incomplete line) in buffer
	int	leftover = strlen(start);
	if (leftover == 0)
	{
		free(clients[fd].buf);
		clients[fd].buf = NULL;
	}
	else
	{
		memmove(clients[fd].buf, start, leftover + 1);
		char	*trimmed = realloc(clients[fd].buf, leftover + 1);
		if (!trimmed)
			fatal();
		clients[fd].buf = trimmed;
	}
}

int	main(int argc, char **argv)
{
	if (argc != 2)
	{
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}

	int	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0)
		fatal();

	struct sockaddr_in	addr;
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(argv[1]));
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		fatal();
	if (listen(server_fd, 128) < 0)
		fatal();

	bzero(clients, sizeof(clients));
	FD_ZERO(&active_fds);
	FD_SET(server_fd, &active_fds);
	max_fd = server_fd;

	while (1)
	{
		fd_set	read_fds = active_fds;
		if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0)
			continue ;
		for (int fd = 0; fd <= max_fd; fd++)
		{
			if (!FD_ISSET(fd, &read_fds))
				continue ;
			if (fd == server_fd)
				client_connect(server_fd);
			else
				process_message(fd);
		}
	}
	return (0);
}