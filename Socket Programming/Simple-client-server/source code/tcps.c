#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>

#define SERVER_TCP_PORT 7005 // Default port
#define SERVER_FILE_TRANSFER_PORT 7006
#define BUFLEN 80 //Buffer length
#define MAX_CLIENT 10
#define TRUE 1
#define LENGTH 512

struct request
{
	int used;
	char type[BUFLEN];
	char filename[BUFLEN];
	char ip[BUFLEN];
};

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
	int opt = TRUE;
	int addrlen, max_sd, activity, valread;
	int sd_7005, sd_7006, client_socket[MAX_CLIENT], sd, new_socket;
	int max_clients = MAX_CLIENT, i = 0;

	char delim[] = " ";
	char buffer[BUFLEN];

	fd_set readfds;

	struct request list[MAX_CLIENT] = {{0}};
	struct sockaddr_in address;

	// initialize all client_socket[] to 0
	for (i = 0; i < max_clients; i++)
	{
		client_socket[i] = 0;
	}

	// create 7005, 7006 socket
	if ((sd_7005 = socket(AF_INET, SOCK_STREAM, 0)) == 0 ||
		(sd_7006 = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("socket failed");
		exit(1);
	}

	// set 7005, 7006 socket to allow multiple connection
	if (setsockopt(sd_7005, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
	{
		perror("7005 setsockopt");
		exit(1);
	}
	if (setsockopt(sd_7006, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
	{
		perror("7006 setsockopt");
		exit(1);
	}

	// Bind an address to the socket
	bzero((char *)&address, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY); // Accept connections from any client

	// bind 7005 port
	address.sin_port = htons(SERVER_TCP_PORT);
	if (bind(sd_7005, (struct sockaddr *)&address, sizeof(address)) < 0)
	{
		perror("bind failed");
		exit(1);
	}

	printf("port %d opened\n", SERVER_TCP_PORT);
	if (listen(sd_7005, 5) < 0)
	{
		perror("listen 7005 failed");
		exit(1);
	}

	// bind 7006 port
	address.sin_port = htons(SERVER_FILE_TRANSFER_PORT);
	if (bind(sd_7006, (struct sockaddr *)&address, sizeof(address)) < 0)
	{
		perror("bind failed");
		exit(1);
	}

	printf("port %d opened\n\n", SERVER_FILE_TRANSFER_PORT);
	if (listen(sd_7006, 5) < 0)
	{
		perror("listen 7005 failed");
		exit(1);
	}

	addrlen = sizeof(address);
	while (TRUE)
	{
		// clear the socket set
		FD_ZERO(&readfds);

		// add sd_7005, sd_7006 to set
		FD_SET(sd_7005, &readfds);
		FD_SET(sd_7006, &readfds);
		max_sd = (sd_7005 > sd_7006 ? sd_7005 : sd_7006);

		for (i = 0; i < max_clients; i++)
		{
			// socket descriptor
			sd = client_socket[i];

			// if valid socket descriptor then add to read list
			if (sd > 0)
				FD_SET(sd, &readfds);

			if (sd > max_sd)
				max_sd = sd;
		}

		activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
		if ((activity < 0) && (errno != EINTR))
		{
			printf("select error");
		}

		// connection to 7005 occured
		if (FD_ISSET(sd_7005, &readfds))
		{
			if ((new_socket = accept(sd_7005, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
			{
				perror("accept failed");
				exit(1);
			}

			int n = write(new_socket, "7005 connection established", BUFLEN);
			if (n < 0)
			{
				perror("ERROR writing to socket");
			}

			printf("%-13s:7005\n%-13s:%s\n%-13s:%d\n\n",
				   "server port",
				   "client IP", inet_ntoa(address.sin_addr),
				   "client port", ntohs(address.sin_port));

			for (i = 0; i < max_clients; i++)
			{
				if (client_socket[i] == 0)
				{
					client_socket[i] = new_socket;
					break;
				}
			}
		}

		// connection to 7006 occured
		else if (FD_ISSET(sd_7006, &readfds))
		{
			if ((new_socket = accept(sd_7006, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
			{
				perror("accept failed");
				exit(1);
			}

			int n = write(new_socket, "7006 connection established\n", BUFLEN);
			if (n < 0)
			{
				perror("ERROR writing to socket");
			}

			printf("%-13s:7006\n%-13s:%s\n%-13s:%d\n\n",
				   "server port",
				   "client IP", inet_ntoa(address.sin_addr),
				   "client port", ntohs(address.sin_port));

			// check request list
			char *ip = inet_ntoa(address.sin_addr);
			for (int i = 0; i < MAX_CLIENT; i++)
			{
				if ((strcmp(ip, list[i].ip)) == 0)
				{
					list[i].used = 0;
					if ((strcmp(list[i].type, "GET")) == 0)
					{
						send_file(new_socket, list[i].filename);
						printf("%-13s:%s\n", "send", list[i].filename);
						printf("%-13s:%s\n\n", "to", ip);
					}
					else if ((strcmp(list[i].type, "SENT")) == 0)
					{
						save_file(new_socket, list[i].filename);
						printf("%-13s:%s\n", "received", list[i].filename);
						printf("%-13s:%s\n\n", "from", ip);
					}
					break;
				}
			}

			for (i = 0; i < max_clients; i++)
			{
				if (client_socket[i] == 0)
				{
					client_socket[i] = new_socket;
					break;
				}
			}
		}
		else
		{
			for (i = 0; i < max_clients; i++)
			{
				sd = client_socket[i];
				if (FD_ISSET(sd, &readfds))
				{
					if ((valread = read(sd, buffer, BUFLEN)) == 0)
					{
						close(sd);
						client_socket[i] = 0;
					}
					else
					{
						char tmp[BUFLEN];
						strcpy(tmp, buffer);
						char *cmd = strtok(tmp, delim);
						char *filename = strtok(NULL, delim);
						getpeername(sd, (struct sockaddr *)&address, (socklen_t *)&addrlen);

						for (int i = 0; i < MAX_CLIENT; i++)
						{
							if (!list[i].used)
							{
								struct request temp;
								temp.used = 1;
								strcpy(temp.type, cmd);
								strcpy(temp.filename, filename);
								strcpy(temp.ip, inet_ntoa(address.sin_addr));
								list[i] = temp;
								break;
							}
						}
						char msg[BUFLEN];
						snprintf(msg, sizeof msg, "server received %s %s request\n", cmd, filename);
						int n = write(sd, msg, BUFLEN);
						if (n < 0)
						{
							perror("ERROR writing to socket");
						}
					}
				}
			}
		}
	}
	return (0);
}
