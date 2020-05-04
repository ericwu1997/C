#ifndef PORT_FORWARD_H
#define PORT_FORWARD_H

#define EPOLL_QUEUE_LEN 256
#define NUM_CORE 4
#define TRUE 1

/*
 * Function:  Print error 
 * --------------------
 * print error
 *
 */
void SystemFatal(const char *message);

/*
 * Function:  port_forwarding_setup 
 * --------------------
 * create a epoll instance and register a listening socket
 * handles icoming traffic
 * 
 * sd: listening socket
 *
 */
void port_forwarding_setup(int sd, char* dst_ip, int dst_port);

/*
 * Function:  offload_forwarding 
 * --------------------
 * create a child process to handle communication 
 * between internal and external client
 * 
 * in_port: internal client port
 * in_ip: internal client ip
 * ex_sd: external client socket
 */
void offload_forwarding(int in_port, char* in_ip, int ex_sd);

#endif