/*---------------------------------------------------------------------------------------
--	SOURCE FILE:    server.c -   A simple port forwarding server
--
--	PROGRAM:        server.exe
--
--	DATE:			Mar 31, 2020
--
--	DESIGNERS:		Eric Wu, Hong Kit Wu
--
--	PROGRAMMERS:	Eric Wu, Hong Kit Wu
--
--	NOTES:
--	The program will forward traffic from client machines. The
--  forwarding destination depends the IP: port pair defined 
--  in the headerfile "port_forward.h"
---------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <strings.h>
#include <string.h>

#include "./lib/connect_setup.h"
#include "./lib/port_forward.h"

int main(int argc, char **argv)
{
    int sd, port,dst_port;
    char dst_ip[16];

    switch (argc)
    {
    case 4:
        port=atoi(argv[1]);
        strcpy(dst_ip, argv[2]);
        dst_port = atoi(argv[3]); // Get user specified port
        break;
    default:
        fprintf(stderr, "Usage: %s [port]\n", argv[0]);
        exit(1);
    }

    // Create a stream socket
    sd = create_socket();
    // Make the server listening socket non-blocking
    if (fcntl(sd, F_SETFL, O_NONBLOCK | fcntl(sd, F_GETFL, 0)) == -1)
    {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }

    // Bind an address to the socket
    bind_socket(sd, port);

    // Listen for connections, queue up to 5 connect requests
    listen(sd, 5);

    // port forwarding setup
    port_forwarding_setup(sd,dst_ip,dst_port);

    exit(EXIT_SUCCESS);
}
