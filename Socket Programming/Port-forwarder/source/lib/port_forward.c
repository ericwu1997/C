#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <omp.h>
#include <assert.h>

#include "port_forward.h"
#include "connect_setup.h"

void port_forwarding_setup(int sd, char* dst_ip, int dst_port)
{
    // Create the epoll file descriptor
    size_t epoll_fd = epoll_create(EPOLL_QUEUE_LEN);
    if (epoll_fd == -1)
        SystemFatal("epoll_create");

    static struct epoll_event event;
    struct epoll_event events[EPOLL_QUEUE_LEN];
    size_t num_fds, fd_new;
    size_t i;

    struct sockaddr_in remote_addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);

    event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET | EPOLLEXCLUSIVE;
    event.data.fd = sd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sd, &event) == -1)
        SystemFatal("1-epoll_ctl");

    omp_set_num_threads(NUM_CORE);
    int EAGAIN_REACHED;

    while (TRUE)
    {
        num_fds = epoll_wait(epoll_fd, events, EPOLL_QUEUE_LEN, -1);
        if (num_fds < 0)
            SystemFatal("Error in epoll_wait!");

        for (i = 0; i < num_fds; i++)
        {
            // Cleint disconnect condition
            if (events[i].events & EPOLLHUP)
            {
                close(events[i].data.fd);
                printf("Remote Disconnect\n");
                continue;
            }

            // Error condition
            if (events[i].events & EPOLLERR)
            {
                fputs("epoll: EPOLLERR\n", stderr);
                close(events[i].data.fd);
                continue;
            }
            assert(events[i].events & EPOLLIN);

            // Server is receiving a connection request
            if (events[i].data.fd == sd)
            {
                EAGAIN_REACHED = 0;
#pragma omp parallel private(fd_new)
                {
                    while (!EAGAIN_REACHED)
                    {
                        fd_new = accept(sd, (struct sockaddr *)&remote_addr, &addr_size);
                        if (fd_new == -1)
                        {
                            if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
                            {
                                EAGAIN_REACHED = 1;
                                break; // We have processed all incoming connections.
                            }
                            else
                            {
                                perror("accept");
                                break;
                            }
                        }

                        // Make the fd_new non-blocking
                        if (fcntl(fd_new, F_SETFL, O_NONBLOCK | fcntl(fd_new, F_GETFL, 0)) == -1)
                            SystemFatal("fcntl");

                        // Add the new socket descriptor to the epoll loop
                        event.data.fd = fd_new;
                        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd_new, &event) == -1)
                            SystemFatal("2-epoll_ctl");
                    }
                }
                continue;
            }

            // Socket ready for reading
            int fd = events[i].data.fd;
            // reset socket to blocking mode
            if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) ^ O_NONBLOCK) == -1)
                SystemFatal("fcntl");
            // remove socket form monitoring list
            if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &event) == -1)
                SystemFatal("3-epoll_ctl");
            pid_t childpid = fork();
            if (childpid == -1)
            {
                perror("Fork error!");
                exit(1);
            }
            if (childpid == 0)
            {
                offload_forwarding(dst_port, dst_ip, fd);
            }
        }
    }

    close(sd);
}

void com(int src, int dst)
{
    char buf[TCP_PACKET_BUFFER];
    int n;
    while ((n = recv(src, buf, TCP_PACKET_BUFFER, 0)) != 0)
    {
        send(dst, buf, n, 0);
    }
    close(src);
}

void offload_forwarding(int in_port, char *in_ip, int ex_sd)
{
    int server_sd = create_socket();
    establish_forward_connection(in_port, in_ip, server_sd);

    pid_t childpid = fork();
    if (childpid == -1)
    {
        perror("Fork error!");
        exit(1);
    }
    if (childpid == 0)
    {
        com(ex_sd, server_sd);
    }
    else
    {
        com(server_sd, ex_sd);
    }
    exit(0);
}

// Prints the error stored in errno and aborts the program.
void SystemFatal(const char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}