/*---------------------------------------------------------------------------------------
--	SOURCE FILE:	env.h -   environment variable for receiver.c transmitter.c and emulator.c
--
--	DATE:			Nov 24, 2019
--
--	DESIGNERS:		Eric Wu, Hong Kit Wu
--
--	PROGRAMMERS:	Eric Wu, Hong Kit Wu
--
--	NOTES:
--	This header file contains macro for port number, ip that are used in receiver.c
--  transmitter.c and emulator.c
---------------------------------------------------------------------------------------*/

#define DEFAULT_RECEIVER_IP "192.168.0.39"
#define DEFAULT_TRANSMITTER_IP "192.168.0.41"
#define DEFAULT_EMULATOR_IP "192.168.0.38"
#define DEFAULT_PORT 5000
#define DEFAULT_DROP_RATE 10
#define DEFAULT_SYN_INTERVAL 100000 //sec
#define DEFAULT_PAYLOAD_SIZE 100
#define DEFAULT_WND_SIZE 4
#define DEFAULT_TEST_FILE "test.txt"
#define DEFAULT_PACKET_DELAY 0