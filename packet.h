#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sstream>
#include <fstream>
#include <time.h>
#include <iostream>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <utility>
#include <signal.h>

const int DATASIZE = 800;

enum PACKET_TYPE {
	NONE = 0,
	FILE_TRANSFER_REQUEST,
	FILE_DATA,
	ACK,
	FILE_NOTEXIST_ERROR,
	FILE_CORRUPTION,
	FILE_TRANSFER_COMPLETE,
	TRANSFER_COMPLETE_ACK
};

struct PACKET{

	PACKET_TYPE type;
	int seqNum;
	int ackNum;
	char payload[DATASIZE] ; 
	PACKET() : type(NONE), seqNum(-1), ackNum(-1){}
};

const int headSize = sizeof(PACKET_TYPE) + sizeof(int) * 3;
const int packetSize = headSize + DATASIZE;
PACKET Packet;