// UDP client program
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "env.h"
#include "packet.h"

int main(int argc, char **argv)
{
	int isDupAck; // Flag for duplicate ack
	int sockfd;
	char buffer[MAXLINE];
	struct sockaddr_in emuladdr; // receiver socket address
	socklen_t len;

	long elasped;
	long rtt;
	long cal_timeout;
	long trans_delay;
	struct timespec start, stop;
	struct timespec begin, completed;
	struct timeval timeout_val; // timeout value

	int sequence_num = 0; // Sequence number
	int last_seq_of_frame = 0;
	int ack_num = 0; // ACK number
	int acked_seq = 0;
	struct Packet *frame[DEFAULT_WND_SIZE]; // frame
	struct Packet *tmp;						// for logging

	int port;
	long syn_interval;
	int payload_size;
	int packet_delay;
	char *filename;
	char rec_ip[16], trans_ip[16], emu_ip[16];

	/*************************************************************/
	/* port			: emulator port								 */
	/* syn_interval	: SYN sending interval						 */
	/* payload_size	: payload size								 */
	/* filename		: test file name							 */
	/* packet delay	: packet delay								 */
	/*************************************************************/
	switch (argc)
	{
	case 1:
		port = DEFAULT_PORT;
		syn_interval = DEFAULT_SYN_INTERVAL;
		payload_size = DEFAULT_PAYLOAD_SIZE;
		filename = DEFAULT_TEST_FILE;
		packet_delay = DEFAULT_PACKET_DELAY;
		break;
	case 2:
		port = DEFAULT_PORT;
		syn_interval = DEFAULT_SYN_INTERVAL;
		filename = DEFAULT_TEST_FILE;
		packet_delay = DEFAULT_PACKET_DELAY;
		payload_size = atoi(argv[1]);
		break;
	case 3:
		port = DEFAULT_PORT;
		syn_interval = DEFAULT_SYN_INTERVAL;
		packet_delay = DEFAULT_PACKET_DELAY;
		payload_size = atoi(argv[1]);
		filename = argv[2];
		break;
	case 4:
		port = DEFAULT_PORT;
		syn_interval = DEFAULT_SYN_INTERVAL;
		payload_size = atoi(argv[1]);
		filename = argv[2];
		packet_delay = atoi(argv[3]);
		break;
	case 5:
		port = DEFAULT_PORT;
		payload_size = atoi(argv[1]);
		filename = argv[2];
		packet_delay = atoi(argv[3]);
		syn_interval = atoi(argv[4]);
		break;
	case 6:
		payload_size = atoi(argv[1]);
		filename = argv[2];
		packet_delay = atoi(argv[3]);
		syn_interval = atoi(argv[4]);
		port = atoi(argv[5]);
		break;
	default:
		fprintf(stderr, "Usage: %s [payload size] [filename] [packet delay (ns)] [syn interval] [port]\n", argv[0]);
		exit(1);
	}

	/*************************************************************/
	/* IP for receiver, emulator and transmitter				 */
	/*************************************************************/
	strcpy(emu_ip, DEFAULT_EMULATOR_IP);
	strcpy(trans_ip, DEFAULT_TRANSMITTER_IP);
	strcpy(rec_ip, DEFAULT_RECEIVER_IP);

	/*************************************************************/
	/* Creating socket file descriptor				     	     */
	/*************************************************************/
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("socket creation failed");
		exit(0);
	}

	/*************************************************************/
	/* Filling emulator information					     	     */
	/*************************************************************/
	memset(&emuladdr, 0, sizeof(emuladdr));
	emuladdr.sin_family = AF_INET;
	emuladdr.sin_port = htons(port);
	emuladdr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sockfd, (struct sockaddr *)&emuladdr, sizeof(emuladdr)) == -1)
	{
		perror("Can't bind name to socket");
		exit(1);
	}
	len = sizeof(emuladdr);
	emuladdr.sin_addr.s_addr = inet_addr(emu_ip);

	/*************************************************************/
	/* Initialize three way handshake					         */
	/*************************************************************/
	size_t i;
	for (i = 0; i < DEFAULT_WND_SIZE; i++)
	{
		frame[i] = create_packet(trans_ip, rec_ip, 0, 0, 0, 0, payload_size);
	}
	frame[0]->PacketType = SYN;
	frame[0]->AckNum = 0;
	frame[0]->SeqNum = 0;
	frame[0]->WindowSize = DEFAULT_WND_SIZE;

	/*************************************************************/
	/* Sent SYN repeatly untill SYN/ACK received				 */
	/*************************************************************/
	timeout_val.tv_sec = 0;
	timeout_val.tv_usec = syn_interval;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout_val, sizeof(timeout_val));
	for (;;)
	{
		/*************************************************************/
		/* Initial transmission delay								 */
		/*************************************************************/
		clock_gettime(CLOCK_REALTIME, &start);
		send_packet(sockfd, frame[0], &emuladdr);
		clock_gettime(CLOCK_REALTIME, &stop);
		trans_delay = stop.tv_nsec - start.tv_nsec;

		/*************************************************************/
		/* RTT delay												 */
		/*************************************************************/
		clock_gettime(CLOCK_REALTIME, &start);
		if (recvfrom(sockfd, (char *)buffer, MAXLINE, 0, (struct sockaddr *)&emuladdr, &len) < 0)
		{
			printf("no response, sending SYN again\n");
		}
		else
		{
			clock_gettime(CLOCK_REALTIME, &stop);
			rtt = (stop.tv_nsec - start.tv_nsec) * 1.25;
			printf("SYN/ACK received\n");

			/*************************************************************/
			/* Calculate initial timeout value							 */
			/*************************************************************/
			cal_timeout = trans_delay * DEFAULT_WND_SIZE + rtt;
			break;
		}
	}

	/*************************************************************/
	/* record transmission time									 */
	/*************************************************************/
	clock_gettime(CLOCK_REALTIME, &begin);

	/*************************************************************/
	/* SYN/ACK received, deserialize and write to log			 */
	/* responde with an ACK informing receiver ACK for SYN/ACK	 */
	/* received													 */
	/*************************************************************/
	tmp = deserialize(buffer, payload_size);
	acked_seq = tmp->AckNum; // 1
	ack_num = tmp->AckNum;

	char pakcet_into[MAXLINE];
	sprintf(pakcet_into, "Src: %s, Des: %s, Packet Type: %10s, SeqNum: %5d, Ack: %2d, Window Size: %2d, Data: %10s",
			tmp->src, tmp->des, parse_type(tmp->PacketType), tmp->SeqNum, tmp->AckNum, tmp->WindowSize, tmp->data);
	write_to_log(pakcet_into, "log.txt");

	tmp->PacketType = ACK;
	send_packet(sockfd, tmp, &emuladdr);

	bzero(buffer, sizeof(buffer));
	free_packet(tmp);

	/*************************************************************/
	/* Open file for reading									 */
	/*************************************************************/
	char sbuf[payload_size];
	FILE *fs = fopen(filename, "r");
	if (fs == NULL)
	{
		printf("ERROR: file %s not found.\n", filename);
		exit(1);
	}
	bzero(sbuf, payload_size);

	int fs_block_sz;

	/*************************************************************/
	/* Start actual transmission								 */
	/*************************************************************/
	size_t end = 0;
	sequence_num++;
	while (!end)
	{
		/*************************************************************/
		/* Read date to frame										 */
		/*************************************************************/
		int i = 0;
		int count = 0;
		for (; i < DEFAULT_WND_SIZE; i++)
		{
			sequence_num = ftell(fs) + 1;
			if ((fs_block_sz = fread(sbuf, sizeof(char), payload_size - 1, fs)) > 0)
			{
				memset(frame[i]->data, '\0', payload_size);
				strcpy(frame[i]->data, sbuf);
				frame[i]->SeqNum = sequence_num;
				frame[i]->AckNum = ack_num;
				frame[i]->PacketType = ACK;
				bzero(sbuf, payload_size);
				count++;
			}
			else
			{
				end = 1;
				i = DEFAULT_WND_SIZE; // exit loop early if no more data to read
			}
		}

		/*************************************************************/
		/* Send data by frame, and record sequence number of last 	 */
		/* sent packet of a frame. Then the value is used to		 */
		/* determine if all packet received if an implicit ACK occur */
		/*************************************************************/
		for (i = 0; i < count; i++)
		{
			clock_gettime(CLOCK_REALTIME, &start);
			send_packet(sockfd, frame[i], &emuladdr);
			clock_gettime(CLOCK_REALTIME, &stop);
			/*************************************************************/
			/* Calculate transmission delay								 */
			/*************************************************************/
			trans_delay = ((stop.tv_nsec - start.tv_nsec) + trans_delay) / 2;
			last_seq_of_frame = frame[i]->SeqNum;
			usleep(packet_delay);
		}

		timeout_val.tv_sec = 0;
		timeout_val.tv_usec = cal_timeout * 0.001; // timeout change to micro sec

		for (i = 0; i < count; i++)
		{
			setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout_val, sizeof(timeout_val));
			clock_gettime(CLOCK_REALTIME, &start);
			if (recvfrom(sockfd, (char *)buffer, MAXLINE, 0, (struct sockaddr *)&emuladdr, &len) < 0)
			{
				/*************************************************************/
				/* Timeout occured, transmite new frame starting from last	 */
				/* ACK														 */
				/*************************************************************/
				fseek(fs, acked_seq - 1, SEEK_SET);
				// printf("Timeout, start from seq: %d\n", acked_seq); // <- console printing
				i = count;
				end = 0; // avoid early exit due to file descriptor reaches end
				continue;
			}
			else
			{
				/*************************************************************/
				/* calculate elapsed time to determine next timoutout value	 */
				/*************************************************************/
				clock_gettime(CLOCK_REALTIME, &stop);
				elasped = stop.tv_nsec - start.tv_nsec;
				timeout_val.tv_usec = timeout_val.tv_usec - (elasped * 0.001);
				cal_timeout = (0.875 * rtt + 0.125 * elasped) +
							  trans_delay * DEFAULT_WND_SIZE +
							  packet_delay * (DEFAULT_WND_SIZE - 1);
				rtt = elasped;

				/*************************************************************/
				/* deserialize new packet 									 */
				/*************************************************************/
				tmp = deserialize(buffer, payload_size);

				/*************************************************************/
				/* if is an ACK packet, process it, otherwise disregard      */
				/*************************************************************/
				if (tmp->PacketType == ACK)
				{
					/*************************************************************/
					/* Ack received, check if duplicate ACK						 */
					/* stop waiting for ACK and resending new frame starting 	 */
					/* from last ACK											 */
					/*************************************************************/
					if (acked_seq == tmp->AckNum)
					{
						isDupAck = 1;
						/*************************************************************/
						/* Duplicate ACK received, set next frame starting point to  */
						/* last missing packet										 */
						/*************************************************************/
						fseek(fs, acked_seq - 1, SEEK_SET);
						// printf("Duplicate, start from %ld, seq: %d\n", ftell(fs), acked_seq); // <- console printing
						i = count;
						end = 0; // avoid early exit due to file descriptor reaches end
					}
					/*************************************************************/
					/* Even if received ACK is greater than last received ACK 	 */
					/* mark it (implicit Acknowledgment) 						 */
					/*************************************************************/
					else if (tmp->AckNum > acked_seq)
					{
						isDupAck = 0; // Flag for duplicate ack
						/*************************************************************/
						/* Even if received ACK is greater than last received ACK 	 */
						/* mark it (implicit Acknowledgment). If is the last  		 */
						/* sequence number of a frame received, exit the waiting 	 */
						/*************************************************************/
						acked_seq = tmp->AckNum;
						if (acked_seq == last_seq_of_frame)
						{
							i = count;
						}
						// printf("Successful transmission, seq: %d\n", acked_seq); // <- console printing
					}
				}
				else
				{
					i--; // If receive something else, wait one more
				}

				/*************************************************************/
				/* write to log												 */
				/*************************************************************/
				char pakcet_into[MAXLINE];
				sprintf(pakcet_into, "Src: %s, Des: %s, Packet Type: %10s, SeqNum: %5d, Ack: %2d, Window Size: %2d, Data: %10s",
						tmp->src, tmp->des, parse_type((tmp->PacketType) + isDupAck), tmp->SeqNum, tmp->AckNum, tmp->WindowSize, tmp->data);
				write_to_log(pakcet_into, "log.txt");

				bzero(buffer, sizeof(buffer));
				free_packet(tmp); // important, free allocated memory
			}
		}
	}

	/*************************************************************/
	/* Inform receiver end of transmission. Send EOT every 1     */
	/* second until receive an ACK from receiver				 */
	/*************************************************************/
	timeout_val.tv_sec = 0;
	timeout_val.tv_usec = syn_interval;

	memset(frame[0]->data, '\0', payload_size);
	frame[0]->SeqNum = sequence_num++;
	frame[0]->AckNum = ack_num;
	frame[0]->PacketType = EOT;

	/*************************************************************/
	/* record transmission time									 */
	/*************************************************************/
	clock_gettime(CLOCK_REALTIME, &completed);
	elasped = (completed.tv_nsec - begin.tv_nsec) + (completed.tv_sec - begin.tv_sec) * 1000000000;
	printf("Transmission time: %ld nsec \n", elasped);

	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout_val, sizeof(timeout_val));
	for (;;)
	{
		send_packet(sockfd, frame[0], &emuladdr);
		if (recvfrom(sockfd, (char *)buffer, MAXLINE, 0, (struct sockaddr *)&emuladdr, &len) < 0)
		{
			printf("no response, sending EOT again\n");
		}
		else
		{
			tmp = deserialize(buffer, payload_size);
			if (tmp->PacketType == EOT)
			{

				printf("transmission completed\n");
				free(tmp);
				break;
			}
			free(tmp);
		}
	}

	/*************************************************************/
	/* free allocated memory									 */
	/*************************************************************/
	for (i = 0; i < DEFAULT_WND_SIZE; i++)
	{
		free_packet(frame[i]);
	}

	close(sockfd); //close connection
	return 0;
}