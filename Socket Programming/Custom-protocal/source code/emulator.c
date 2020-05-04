// Server program
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include "env.h"
#include "packet.h"

#define MAXLINE 1024

int main(int argc, char **argv)
{
	time_t t;
	double n;
	srand((unsigned)time(&t)); // random

	int sd;
	char buffer[MAXLINE];
	socklen_t len;
	struct sockaddr_in cliaddr, servaddr;
	void sig_chld(int);

	int port;
	int drop_rate;
	/*************************************************************/
	/* port		: receiver port									 */
	/*************************************************************/
	switch (argc)
	{
	case 1:
		port = DEFAULT_PORT;
		drop_rate = DEFAULT_DROP_RATE;
		break;
	case 2:
		port = atoi(argv[1]); //User specified port
		drop_rate = DEFAULT_DROP_RATE;
		break;
	case 3:
		port = atoi(argv[1]); //User specified port
		drop_rate = atoi(argv[2]);
		break;
	default:
		fprintf(stderr, "Usage: %s [port] emulator ip, transmitter ip, receiver ip\n", argv[0]);
		exit(1);
	}

	/*************************************************************/
	/* Create a datagram socket						             */
	/*************************************************************/
	if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		perror("Can't create a socket");
		exit(1);
	}

	/*************************************************************/
	/* Bind an address to the socket				             */
	/*************************************************************/
	bzero((char *)&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
	{
		perror("Can't bind name to socket");
		exit(1);
	}
	len = sizeof(servaddr);

	for (;;)
	{
		bzero(buffer, sizeof(buffer));

		recvfrom(sd, buffer, sizeof(buffer), 0,
				 (struct sockaddr *)&cliaddr, &len);

		/*************************************************************/
		/* Simulate random drop										 */
		/*************************************************************/
		if ((n = rand() % 100) + 1 >= drop_rate)
		{
			char *addr = buffer + DES_IP_OFFSET;

			printf("From: %s, to %s\n", buffer + SRC_IP_OFFSET, buffer + DES_IP_OFFSET);

			cliaddr.sin_addr.s_addr = inet_addr(addr);
			cliaddr.sin_port = htons(DEFAULT_PORT);

			if (sendto(sd, (char *)buffer, sizeof(buffer), 0,
					   (struct sockaddr *)&cliaddr, sizeof(cliaddr)) == -1)
			{
				perror("redirect failed\n");
				exit(1);
			}
		}
	}

	return 0;
}