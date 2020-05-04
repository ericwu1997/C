#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "connect_setup.h"

int create_socket()
{
    int sd;
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Can't create a socket");
        return -1;
    }
    int enable = 1;
    if (setsockopt(sd, SOL_SOCKET, (SO_REUSEADDR | SO_REUSEPORT), &enable, sizeof(int)) < 0)
    {
        perror("setsockopt(SO_REUSEADDR) failed");
        return -1;
    }

    return sd;
}

int bind_socket(int socket, int port)
{
    struct sockaddr_in server;
    bzero((char *)&server, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(socket, (struct sockaddr *)&server, sizeof(server)) == -1)
    {
        perror("Can't bind name to socket");
        return 1;
    }
    return 0;
}

int establish_forward_connection(int server_port, char *server_ip, int socket)
{
    struct sockaddr_in server;
    struct hostent *hp;
    bzero((char *)&server, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(server_port);
    if ((hp = gethostbyname(server_ip)) == NULL)
    {
        fprintf(stderr, "Unknown server address\n");
        return -1;
    }
    bcopy(hp->h_addr_list[0], (char *)&server.sin_addr, hp->h_length);

    // Connecting to the server
    if (connect(socket, (struct sockaddr *)&server, sizeof(server)) == -1)
    {
        fprintf(stderr, "Can't connect to server\n");
        perror("connect");
        return -1;
    }
    return 1;
}