/*---------------------------------------------------------------------------------------
--	SOURCE FILE:		tcp_clnt.c - A simple TCP client program.
--
--	PROGRAM:		tclnt.exe
--
--	FUNCTIONS:		Berkeley Socket API
--
--	DATE:			January 23, 2001
--
--	REVISIONS:		(Date and Description)
--				January 2005
--				Modified the read loop to use fgets.
--				While loop is based on the buffer length 
--
--
--	DESIGNERS:		Aman Abdulla
--
--	PROGRAMMERS:		Aman Abdulla
--
--	NOTES:
--	The program will establish a TCP connection to a user specifed server.
-- 	The server can be specified using a fully qualified domain name or and
--	IP address. After the connection has been established the user will be
-- 	prompted for date. The date string is then sent to the server and the
-- 	response (echo) back from the server is displayed.
---------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

#define SERVER_TCP_PORT 7005
#define SERVER_FILE_TRANSFER_PORT 7006 // Default port
#define BUFLEN 80					   // Buffer length
#define LENGTH 512

void send_file(int sd, char *filename)
{
	char sdbuf[LENGTH];
	FILE *fs = fopen(filename, "r");
	if (fs == NULL)
	{
		printf("ERROR: file %s not found.\n", filename);
		exit(1);
	}
	bzero(sdbuf, LENGTH);
	int fs_block_sz;
	while ((fs_block_sz = fread(sdbuf, sizeof(char), LENGTH, fs)) > 0)
	{
		if (send(sd, sdbuf, fs_block_sz, 0) < 0)
		{
			printf("ERROR: failed to send file %s.\n", filename);
			break;
		}
		bzero(sdbuf, LENGTH);
	}
}

void save_file(int sd, char *filename)
{
	char revbuf[LENGTH];
	int fr_block_sz = 0;
	FILE *fr = fopen(filename, "w+");
	if (fr == NULL)
		printf("file %s Cannot be opened.\n", filename);
	while ((fr_block_sz = recv(sd, revbuf, LENGTH, 0)))
	{
		if (fr_block_sz < 0)
		{
			perror("receive file error.\n");
		}
		int write_sz = fwrite(revbuf, sizeof(char), fr_block_sz, fr);
		if (write_sz < fr_block_sz)
		{
			perror("File write failed.\n");
		}
		else if (fr_block_sz)
		{
			break;
		}
		bzero(revbuf, LENGTH);
	}
	fclose(fr);
}

int main(int argc, char **argv)
{
	int opt = 1;
	char *cmd, *filename;
	int n;
	int sd;
	struct hostent *hp;
	struct sockaddr_in server;
	char *host, rbuf[BUFLEN], sbuf[BUFLEN];

	switch (argc)
	{
	case 4:
		host = argv[1]; // Host name
		cmd = argv[2];
		filename = argv[3];
		break;
	default:
		fprintf(stderr, "Usage: %s host action filename\n", argv[0]);
		exit(1);
	}

	// Create the socket
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Cannot create socket");
		exit(1);
	}

	if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
	{
		perror("setsockopt");
		exit(1);
	}

	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(SERVER_TCP_PORT);
	if ((hp = gethostbyname(host)) == NULL)
	{
		fprintf(stderr, "Unknown server address\n");
		exit(1);
	}
	bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);

	// Connecting to the server
	if (connect(sd, (struct sockaddr *)&server, sizeof(server)) == -1)
	{
		fprintf(stderr, "Can't connect to server\n");
		perror("connect");
		exit(1);
	}

	// wait for connection established message
	n = 0;
	n = read(sd, rbuf, BUFLEN);
	if (n < 0)
	{
		perror("ERROR reading from socket");
	}
	printf("%s\n", rbuf);

	// Send request type and filename to server
	snprintf(sbuf, sizeof sbuf, "%s %s", cmd, filename);
	n = write(sd, sbuf, BUFLEN);
	if (n < 0)
		perror("ERROR writing to socket");

	// client makes repeated calls to read until no more data is expected to arrive.
	n = 0;
	n = read(sd, rbuf, BUFLEN);
	if (n < 0)
	{
		perror("ERROR reading from socket");
	}
	printf("%s", rbuf);

	close(sd);
	server.sin_port = htons(SERVER_FILE_TRANSFER_PORT);
	
	// Create the socket
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Cannot create socket");
		exit(1);
	}
	if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
	{
		perror("setsockopt");
		exit(1);
	}
	if (connect(sd, (struct sockaddr *)&server, sizeof(server)) == -1)
	{
		fprintf(stderr, "Can't connect to server\n");
		perror("connect");
		exit(1);
	}
	n = read(sd, rbuf, BUFLEN);
	if (n < 0)
	{
		perror("ERROR reading from socket");
	}
	printf("%s", rbuf);

	if ((strcmp(cmd, "GET")) == 0)
	{
		printf("receiving file...\n");
		save_file(sd, filename);
		printf("file received!\n");
	}
	else if ((strcmp(cmd, "SENT")) == 0)
	{
		printf("sending file to server...\n");
		send_file(sd, filename);
		printf("file sent successful!\n");
	}

	fflush(stdout);
	close(sd);
	return (0);
}