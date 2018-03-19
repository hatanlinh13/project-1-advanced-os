/*
 * HDHNC - Project 1 - Socket Programming
 * Server Side
 *
 * 1512284 - Ha Tan Linh
 */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

#define DEFAULT_PORT 1331
#define MAX_CLIENTS  1024
#define BUF_SIZE     256

void usage_exit(const char *prog_name);
void error_exit(const char *error_msg);

void reset_console(int count, int port);

int main(int argc, char **argv)
{
	int count = 0;
	int lport = 0;
	int server_sock;
	int n_client;
	int client_sock[MAX_CLIENTS];

	/* parsing command line arguments */
	if (argc == 1)
		lport = DEFAULT_PORT;
	else if (argc == 2)
		lport = atoi(argv[1]);
	else
		usage_exit(argv[0]);

	/* init client sockets list to 0, mark as unused */
	n_client = 0;
	for (int i = 0; i < MAX_CLIENTS; i++)
		client_sock[i] = 0;

	/* create server socket */
	if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) <= 0)
		error_exit("Cannot create socket.");
	/* bind the socket to listen port */
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(lport);
	if (bind(server_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		error_exit("Cannot bind socket to listen port.");
	/* start listen for incomming connections */
	if (listen(server_sock, MAX_CLIENTS) < 0)
		error_exit("Cannot listen for incomming connections.");

	fd_set readfds;
	char   buf[BUF_SIZE];
	int    maxfd; /* needed for select call */
	int    addrlen = sizeof(addr);

	reset_console(count, lport);
	while (1) {
		/* clear socket set */
		FD_ZERO(&readfds);

		/* add server socket to set */
		FD_SET(server_sock, &readfds);
		maxfd = server_sock;
		/* add client socket to set */
		for (int i = 0; i < n_client; i++) {
			if (client_sock[i] > 0)
				FD_SET(client_sock[i], &readfds);
			if (client_sock[i] > maxfd)
				maxfd = client_sock[i];
		}

		/* wait for activities from fd set without timeout */
		int act = select(maxfd + 1, &readfds, NULL, NULL, NULL);
		/* check for error of select, except interrupted by a signal */
		if (act < 0 && errno != EINTR)
			error_exit("Problem occurred while waiting for activities.");

		/* input ready on server socket, incomming connections available */
		if (FD_ISSET(server_sock, &readfds)) {
			/* accept the connection */
			int new_sock;
			if ((new_sock = accept(server_sock,
			                      (struct sockaddr *)&addr,
			                      (socklen_t *)&addrlen)) < 0)
				error_exit("Cannot accept new connection.");

			/* add new socket to client list */
			for (int i = 0; i < MAX_CLIENTS; i++)
				if (client_sock[i] == 0) {
					client_sock[i] = new_sock;
					if (i + 1 > n_client)
						n_client = i + 1;

					break;
				}
		}

		/* check for other sockets in set */
		for (int i = 0; i < n_client; i++) {
			int fd = client_sock[i];

			if (FD_ISSET(fd, &readfds)) {
				int num_reiv;
				if ((num_reiv = recv(fd, buf, BUF_SIZE, 0)) > 0) {
					buf[num_reiv] = '\0'; /* ensure null terminated string */
					char *str = buf;
					char *add_occr;
					char *sub_occr;
					while (1) {
						add_occr = strstr(str, "Add");
						if (add_occr) {
							count++;
							reset_console(count, lport);
							str = add_occr + 3;
							continue;
						}
						else {
							sub_occr = strstr(str, "Sub");
							if (sub_occr) {
								count--;
								reset_console(count, lport);
								str = sub_occr + 3;
								continue;
							}
							else {
								break;
							}
						}
					}
				}
				else if (num_reiv == 0) { /* connection closed */
					close(fd);
					client_sock[i] = 0;
				}
				else {
					error_exit("Cannot receive data from socket.");
				}
			}
		}
	}

	return 0;
}

void usage_exit(const char *prog_name)
{
	fprintf(stderr, "Error: Invalid command line arguments!\n");
	fprintf(stdout, "Usage: %s [<listen port>]\n", prog_name);
	exit(EXIT_FAILURE);
}

void error_exit(const char *error_msg)
{
	fprintf(stderr, "Error: %s\n", error_msg);
	exit(EXIT_FAILURE);
}

void reset_console(int count, int port)
{
	system("clear");
	printf("Simple socket server - Listening on port %d\n", port);
	printf("-------------\n");
	printf("count = %d\n", count);
}
