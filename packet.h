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

const int DATASIZE= 1024;

enum PACKET_TYPE {
	NONE=0,
	FILE_TRANSFER_REQUEST,
	FILE_DATA,
	ACK,
	FILE_NOTEXIST_ERROR,
	ERROR,
	FILR_TRANSFER_COMPLETE,
};

struct PACKET{

	PACKET_TYPE type;
	int seq;
	int checksum;
	int ackNum;
	char payload[DATASIZE] ;
	//struct sockaddr_in src_addr;
	//struct sockaddr_in dest_addr;
	//long dest_portNum;

	PACKET():type(NONE), seq(0),checksum(0){}
};

const int headSize=sizeof(PACKET_TYPE)+2*sizeof(int);
const int packetSize=headSize+DATASIZE;
PACKET Packet;