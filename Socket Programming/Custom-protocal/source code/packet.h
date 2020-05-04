#include <string.h>

#define SYN 0
#define SYN_ACK 1
#define PUSH_ACK 2
#define ACK 3
#define DUP_ACK 4
#define EOT 5

#define SRC_IP_BITS 18 //255.255.255.255'\0'
#define DES_IP_BITS 18
#define PACKET_TYPE_BITS 2
#define SEQ_NUM_BITS 10
#define WND_SIZE_BITS 2
#define ACK_NUM_BITS 10

#define SRC_IP_OFFSET 0
#define DES_IP_OFFSET SRC_IP_OFFSET + SRC_IP_BITS
#define PACKET_TYPE_OFFSET DES_IP_OFFSET + DES_IP_BITS
#define SEQ_NUM_OFFSET PACKET_TYPE_OFFSET + PACKET_TYPE_BITS
#define WND_SIZE_OFFSET SEQ_NUM_OFFSET + SEQ_NUM_BITS
#define ACK_NUM_OFFSET WND_SIZE_OFFSET + WND_SIZE_BITS
#define DATA_OFFSET ACK_NUM_OFFSET + ACK_NUM_BITS
#define HEADER_SIZE SRC_IP_BITS + DES_IP_BITS + PACKET_TYPE_BITS + SEQ_NUM_BITS + WND_SIZE_BITS + ACK_NUM_BITS

#define MAXLINE 1024

/*************************************************************/
/* Packet structure                                          */
/*************************************************************/
struct Packet
{
    int PacketType;
    int SeqNum;
    int WindowSize;
    int AckNum;
    char *src;
    char *des;
    char *data;
};

/*************************************************************/
/* Returns struct Packet with provided param set             */
/*************************************************************/
struct Packet *create_packet(char *src, char *des, int type, int seqNum, int windowSize, int ack, int payload_size)
{
    struct Packet *p = (struct Packet *)malloc(sizeof(struct Packet));
    p->src = malloc(strlen(src) + 1);
    strcpy(p->src, src);
    p->des = malloc(strlen(des) + 1);
    strcpy(p->des, des);
    p->PacketType = type;
    p->SeqNum = seqNum;
    p->WindowSize = windowSize;
    p->AckNum = ack;
    p->data = malloc(sizeof(char) * payload_size);
    memset(p->data, '\0', payload_size);
    return p;
}

/*************************************************************/
/* free malloc                                               */
/*************************************************************/
void free_packet(struct Packet *p)
{
    free(p->src);
    free(p->des);
    free(p->data);
    free(p);
}

/*************************************************************/
/* Turn struct Packet to string                              */
/*************************************************************/
char *serialize_packet(struct Packet *p)
{
    int DATAGRAM_SIZE = HEADER_SIZE + strlen(p->data);
    char *serialized_packet = malloc(sizeof(char) * DATAGRAM_SIZE);
    memset(serialized_packet, '\0', DATAGRAM_SIZE);

    strcat(serialized_packet + SRC_IP_OFFSET, p->src);
    strcat(serialized_packet + DES_IP_OFFSET, p->des);
    snprintf(serialized_packet + PACKET_TYPE_OFFSET, PACKET_TYPE_BITS, "%d", p->PacketType);
    snprintf(serialized_packet + SEQ_NUM_OFFSET, SEQ_NUM_BITS, "%d", p->SeqNum);
    snprintf(serialized_packet + WND_SIZE_OFFSET, WND_SIZE_BITS, "%d", p->WindowSize);
    snprintf(serialized_packet + ACK_NUM_OFFSET, ACK_NUM_BITS, "%d", p->AckNum);
    strcat(serialized_packet + DATA_OFFSET, p->data);

    return serialized_packet;
}

/*************************************************************/
/* Turn string into Packet                                   */
/*************************************************************/
struct Packet *deserialize(char *serialized_packet, int payload_size)
{
    char *src = serialized_packet + SRC_IP_OFFSET;
    char *des = serialized_packet + DES_IP_OFFSET;
    char *type_str = serialized_packet + PACKET_TYPE_OFFSET;
    char *seqNum_str = serialized_packet + SEQ_NUM_OFFSET;
    char *wndSize_str = serialized_packet + WND_SIZE_OFFSET;
    char *ack_str = serialized_packet + ACK_NUM_OFFSET;
    char *data = serialized_packet + DATA_OFFSET;

    int type, seqNum, wndSize, ack;
    sscanf(type_str, "%d", &type);
    sscanf(seqNum_str, "%d", &seqNum);
    sscanf(wndSize_str, "%d", &wndSize);
    sscanf(ack_str, "%d", &ack);

    struct Packet *tmp = create_packet(src, des, type, seqNum, wndSize, ack, payload_size);
    strcpy(tmp->data, data);
    return tmp;
}

/*************************************************************/
/* Write packet to socket                                    */
/*************************************************************/
void send_packet(int sd, struct Packet *p, struct sockaddr_in *servaddr)
{
    char *serialized_packet = serialize_packet(p);
    if (sendto(sd, serialized_packet, sizeof(char) * (HEADER_SIZE + strlen(p->data)),
               0, (const struct sockaddr *)servaddr,
               sizeof(*servaddr)) < 0)
    {
        printf("ERROR: failed to send packet\n");
    }
    free(serialized_packet);
}

/*************************************************************/
/* write log to specified file char by char                  */
/*************************************************************/
void write_to_log(char *serialized_packet, char *filepath)
{
    FILE *fs = fopen(filepath, "a+");
    int len = strlen(serialized_packet);
    int i = 0;
    while (len > i)
    {
        char c = *(serialized_packet + i);
        if (c == '\n')
        {
            fputc('\\', fs);
            fputc('n', fs);
        }
        else
        {
            fputc(c, fs);
        }
        i++;
    }
    fputc('\n', fs);

    fclose(fs);
}

/*************************************************************/
/* Parse packet type                                         */
/*************************************************************/
char *parse_type(int type)
{
    switch (type)
    {
    case SYN:
        return "[SYN]";
    case SYN_ACK:
        return "[SYN/ACK]";
    case PUSH_ACK:
        return "[PUSH/ACK]";
    case ACK:
        return "[ACK]";
    case DUP_ACK:
        return "[DUP ACK]";
    case EOT:
        return "[EOT]";
    default:
        return "UNKNOWN";
    }
}