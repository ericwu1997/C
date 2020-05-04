#ifndef CONNECT_SETUP_H
#define CONNECT_SETUP_H

// User define section
// *IP port pair
#define HTTP_SERVER_IP "192.168.0.53"
#define HTTP_PORT 8008
#define HTTPS_SERVER_IP "192.168.0.43"
#define HTTPS_PORT 443
// end of user define section

#define TCP_PACKET_BUFFER 65535

/*
 * Function:  create_socket 
 * --------------------
 * create a socket for use
 *
 *  returns: new socket for use 
 *           returns -1 on error
 */
int create_socket();

/*
 * Function:  bind_socket 
 * --------------------
 * bind socket to port
 *
 *  sd: socket to bind
 *  port: binding port
 *
 *  returns 1 on error (if n is non-positive), 0 on success
 */
int bind_socket(int sd, int port);

/*
 * Function:  establish_forward_connection 
 * --------------------
 * establish connection to internal server
 *
 *  port: internal host port
 *  ip: internal host ip
 *  sd: socket for connection
 *
 *  returns 1 on error (if n is non-positive), 0 on success
 */
int establish_forward_connection(int port, char* ip, int sd);

#endif