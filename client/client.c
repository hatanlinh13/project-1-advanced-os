/*
 * HDHNC - Project 1 - Socket Programming
 * Client Side
 *
 * 1512284 - Ha Tan Linh
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define LINE_MAX     256
#define LOCALHOST    "127.0.0.1"
#define DEFAULT_PORT 1331

struct automode_info {
	int n;
	int interval;
	int pcount;
	int *pattern;
};

void usage_exit(const char *prog_name);
void error_exit(const char *error_msg);

void get_config(char *fname, struct automode_info *inf);
int  parser(int argc, char **argv,
            struct sockaddr_in *serv_addr, struct automode_info *inf);

void interactive_mode(int sockfd);
void automatic_mode(int sockfd, struct automode_info *inf);

int main(int argc, char **argv)
{
	/* connection information */
	int sockfd;
	struct sockaddr_in serv_addr;
	
	/* interactive mode information */
	int interactive;

	/* automatic mode information */
	struct automode_info inf;

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;

	/* parse command line arguments */
	interactive = parser(argc, argv, &serv_addr, &inf);

	/* create and connect socket */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		error_exit("Cannot create socket.");
	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
		error_exit("Cannot connect to server.");

	/* send data to server */
	if (interactive)
		interactive_mode(sockfd);
	else
		automatic_mode(sockfd, &inf);

	if (inf.pattern)
		free(inf.pattern);
	return 0;
}

void usage_exit(const char *prog_name)
{
	fprintf(stderr, "Error: Invalid command line arguments!\n");
	fprintf(stdout, "Usage: %s [<server ip> <server port>] [-c]\n", prog_name);
	exit(EXIT_FAILURE);
}

void error_exit(const char *error_msg)
{
	fprintf(stderr, "Error: %s\n", error_msg);
	exit(EXIT_FAILURE);
}

void get_config(char *fname, struct automode_info *inf)
{
	char buf[LINE_MAX];
	int i;
	FILE *cfg = fopen(fname, "r");
	if (cfg == NULL)
		error_exit("Cannot open config file.");

	if (fgets(buf, LINE_MAX, cfg) == NULL)
		error_exit("Config file syntax error.");
	sscanf(buf, "%d %d %d", &inf->n, &inf->interval, &inf->pcount);

	inf->pattern = (int *)calloc(inf->pcount, sizeof(int));
	if (inf->pattern == NULL)
		error_exit("Cannot allocate memory.");

	for (i = 0; i < inf->pcount; i++) {
		if (fgets(buf, LINE_MAX, cfg) == NULL)
			error_exit("Config file syntax error.");

		if (strncmp(buf, "Add", 3) == 0)
			inf->pattern[i] = 1; /* 1 for add */
		else if (strncmp(buf, "Sub", 3) == 0)
			inf->pattern[i] = 0; /* 0 for sub */
		else
			error_exit("Config file syntax error.");
	}
	fclose(cfg);
}

int parser(int argc, char **argv,
           struct sockaddr_in *serv_addr, struct automode_info *inf)
{
	int rslt = 0;
	switch (argc) {
	case 1:
		if (inet_pton(AF_INET, LOCALHOST, &serv_addr->sin_addr) <= 0)
			error_exit("Invalid server IP address.");
		serv_addr->sin_port = htonl(DEFAULT_PORT);
		rslt = 1;
		break;
	case 3:
		if (strncmp(argv[1], "-c", 2) == 0) {
			if (inet_pton(AF_INET, LOCALHOST, &serv_addr->sin_addr) <= 0)
				error_exit("Invalid server IP address.");
			serv_addr->sin_port = htonl(DEFAULT_PORT);

			get_config(argv[2], inf);
			rslt = 1;
		}
		else {
			if (inet_pton(AF_INET, argv[1], &serv_addr->sin_addr) <= 0)
				error_exit("Invalid server IP address.");
			serv_addr->sin_port = htonl(atoi(argv[2]));
		}
		break;
	case 5:
		if (inet_pton(AF_INET, argv[1], &serv_addr->sin_addr) <= 0)
			error_exit("Invalid server IP address.");
		serv_addr->sin_port = htonl(atoi(argv[2]));
		if (strncmp(argv[3], "-c", 2) == 0) {
			if (inet_pton(AF_INET, LOCALHOST, &serv_addr->sin_addr) <= 0)
				error_exit("Invalid server IP address.");
			serv_addr->sin_port = htonl(DEFAULT_PORT);

			get_config(argv[2], inf);
		}
		else {
			usage_exit(argv[0]);
		}
		break;
	default:
		usage_exit(argv[0]);
	}
	return rslt;
}

void interactive_mode(int sockfd)
{
	char cmd[LINE_MAX];
	printf("Type your command:\n");
	while (1) {
		printf("> ");
		scanf("%s", cmd);
		if (strncmp(cmd, "Exit", 4) == 0) {
			printf("Good bye!\n");
			return;
		}
		else if (strncmp(cmd, "Add", 3) == 0) {
			if (send(sockfd, "Add", 3, 0) == 3)
				printf("Add command sent to server.\n");
			else
				error_exit("Cannot send data.");
		}
		else if (strncmp(cmd, "Sub", 3) == 0) {
			if (send(sockfd, "Sub", 3, 0) == 0)
				printf("Sub command sent to server.\n");
			else
				error_exit("Cannot send data.");
		}
		else {
			printf("Invalid command! Try 'Add', 'Sub' or 'Exit'.\n");
		}
	}
}

void automatic_mode(int sockfd, struct automode_info *inf)
{
	int i     = 1;
	int slide = 0;
	while (1) {
		printf("Turn %d:", i);

		if (inf->pattern[slide] == 1) { /* add */
			if (send(sockfd, "Add", 3, 0) == 3)
				printf("Sent 'Add' to server.\n");
			else
				error_exit("Cannot send data.");
		}
		else { /* sub */
			if (send(sockfd, "Sub", 3, 0) == 3)
				printf("Sent 'Sub' to server.\n");
			else
				error_exit("Cannot send data.");
		}

		slide = (slide + 1) % inf->pcount;
		if (++i > inf->n) break;
		sleep(inf->interval);
	}
	printf("Finished. Goodbye!\n");
}
