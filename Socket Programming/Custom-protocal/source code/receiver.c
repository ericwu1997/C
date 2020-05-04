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
#include "packet.h"
#include "env.h"

#define MAXLINE 1024

int main(int argc, char **argv)
{
	socklen_t len;
	struct sockaddr_in recvaddr, emuladdr;

	char buffer[MAXLINE];
	int sd, seq_num = 0;
	int next_expected = 0;

	struct Packet *tmp; // for logging

	int port;
	int payload_size;

	/*************************************************************/
	/* port			: receiver port								 */
	/* payload_size	: payload size								 */
	/*************************************************************/
	switch (argc)
	{
	case 1:
		payload_size = DEFAULT_PAYLOAD_SIZE;
		port = DEFAULT_PORT;
		break;
	case 2:
		payload_size = atoi(argv[1]); //User specified port
		port = DEFAULT_PORT;
		break;
	case 3:
		payload_size = atoi(argv[1]); //User specified port
		port = atoi(argv[2]);
		break;
	default:
		fprintf(stderr, "Usage: %s [payload size] [port]\n", argv[0]);
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
	bzero((char *)&recvaddr, sizeof(recvaddr));
	recvaddr.sin_family = AF_INET;
	recvaddr.sin_port = htons(port);
	recvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sd, (struct sockaddr *)&recvaddr, sizeof(recvaddr)) == -1)
	{
		perror("Can't bind name to socket");
		exit(1);
	}
	len = sizeof(recvaddr);

	/*************************************************************/
	/* Wait for transmitter initiate 3 way handshake             */
	/*************************************************************/
	printf("Wait for SYN\n");
	recvfrom(sd, buffer, sizeof(buffer), 0,
			 (struct sockaddr *)&emuladdr, &len);

	/*************************************************************/
	/* packet received, deserialize and write to log             */
	/*************************************************************/
	tmp = deserialize(buffer, payload_size);
	char pakcet_into[MAXLINE];
	sprintf(pakcet_into, "Src: %s, Des: %s, Packet Type: %10s, SeqNum: %5d, Ack: %2d, Window Size: %2d, Data: %10s",
			tmp->src, tmp->des, parse_type(tmp->PacketType), tmp->SeqNum, tmp->AckNum, tmp->WindowSize, tmp->data);
	write_to_log(pakcet_into, "log.txt");
	bzero(buffer, sizeof(buffer));

	/*************************************************************/
	/* If is SYN request received, responde with SYN/ACK         */
	/*************************************************************/
	if (tmp->PacketType == SYN) //SYN received
	{
		char *str = tmp->src;
		printf("SYN received\n");
		memset(tmp->data, '\0', payload_size);
		tmp->PacketType = SYN_ACK;
		tmp->src = tmp->des;
		tmp->des = str;
		tmp->SeqNum = seq_num;
		tmp->AckNum = 1; // first byte expected

		send_packet(sd, tmp, &emuladdr);
		free_packet(tmp);

		next_expected = 1; // start from first byte
	}

	/*************************************************************/
	/* If is SYN request received, responde with SYN/ACK         */
	/*************************************************************/
	char *filename = "test.txt";
	FILE *fs = fopen("output.txt", "w");
	if (fs == NULL)
	{
		printf("ERROR: file %s not found.\n", filename);
		exit(1);
	}

	/*************************************************************/
	/* Exit if EOF receive, otherwise keep waiting for new       */
	/* packet													 */
	/*************************************************************/
	for (;;)
	{
		recvfrom(sd, buffer, sizeof(buffer), 0,
				 (struct sockaddr *)&emuladdr, &len);

		/*************************************************************/
		/* packet received, deserialize and write to log             */
		/*************************************************************/
		tmp = deserialize(buffer, payload_size);
		char pakcet_into[MAXLINE];
		sprintf(pakcet_into, "Src: %s, Des: %s, Packet Type: %10s, SeqNum: %5d, Ack: %2d, Window Size: %2d, Data: %10s",
				tmp->src, tmp->des, parse_type(tmp->PacketType), tmp->SeqNum, tmp->AckNum, tmp->WindowSize, tmp->data);
		write_to_log(pakcet_into, "log.txt");
		bzero(buffer, sizeof(buffer));

		/*************************************************************/
		/* If new data received, responde with ACK        			 */
		/* If sequence number is 0, ignore it (0 indicates an ACK    */
		/* for SYN/ACK)												 */
		/*************************************************************/
		if (tmp->PacketType == ACK && (tmp->SeqNum >= 1))
		{
			char *str = tmp->src;
			if (tmp->SeqNum == next_expected)
			{
				next_expected = next_expected + strlen(tmp->data);
				fputs(tmp->data, fs);
			}
			memset(tmp->data, '\0', payload_size);
			tmp->src = tmp->des;
			tmp->des = str;
			tmp->AckNum = next_expected;
			tmp->SeqNum = 1;
			tmp->PacketType = ACK;

			send_packet(sd, tmp, &emuladdr);
			free_packet(tmp);
			continue;
		}

		/*************************************************************/
		/* If is SYN request received, responde with SYN/ACK         */
		/*************************************************************/
		if (tmp->PacketType == SYN) //SYN received
		{
			char *str = tmp->src;
			printf("SYN received\n");
			memset(tmp->data, '\0', payload_size);
			tmp->src = tmp->des;
			tmp->des = str;
			tmp->PacketType = 1;
			tmp->SeqNum = seq_num;
			tmp->AckNum = 1;

			send_packet(sd, tmp, &emuladdr);
			free_packet(tmp);
			continue;
		}

		/*************************************************************/
		/* if EOT received, responde with ACK to inform EOT received */
		/*************************************************************/
		if (tmp->PacketType == EOT)
		{
			char *str = tmp->src;
			printf("EOT\n");
			memset(tmp->data, '\0', payload_size);
			tmp->src = tmp->des;
			tmp->des = str;
			tmp->AckNum = tmp->SeqNum;
			tmp->SeqNum = 1;
			tmp->PacketType = ACK;

			send_packet(sd, tmp, &emuladdr);
			free_packet(tmp);
			break;
		}
	}
	fclose(fs);
	close(sd);
	return 0;
}