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
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <utility>
#include <signal.h>
#include <time.h>

const int DATASIZE = 800;

enum PACKET_TYPE {
	NONE = 0,
	FILE_TRANSFER_REQUEST,
	FILE_DATA,
	ACK,
	FILE_EXISTS,
	FILE_NOTEXIST_ERROR,
	FILE_CORRUPTION,
	FILE_TRANSFER_COMPLETE,
	TRANSFER_COMPLETE_ACK
};

struct PACKET{
	PACKET_TYPE type;
	int seqNum;
	int ackNum;
	int maxSeqNum;
	unsigned int byteSeqNum;
	char payload[DATASIZE]; 
	PACKET() : type(NONE), seqNum(-1), ackNum(-1), maxSeqNum(0), byteSeqNum(0){}
};

const int headSize = sizeof(PACKET_TYPE) + sizeof(int) * 3 + sizeof(unsigned int);
const int packetSize = headSize + DATASIZE;
PACKET Packet;

int udpsend(int sockfd, const void *msg, int len, unsigned int flags,
				const struct sockaddr *to, socklen_t tolen,
				float Pl, float Pc) {
	float corrProb = (float)rand() / (float)RAND_MAX;
	float lossProb = (float)rand() / (float)RAND_MAX;
	bool isCorr = (corrProb < Pc);
	bool isLost = (lossProb < Pl);

	if (isCorr && !isLost) {
		std::cout << "Packet seqNum" << Packet.seqNum <<
		"/" << "ackNum" << Packet.ackNum <<
		" now corrupted!" << std::endl;
		Packet.type = FILE_CORRUPTION;
	} else if (isLost) {
		std::cout << "Packet seqNum" << Packet.seqNum <<
		"/" << "ackNum" << Packet.ackNum <<
		" lost!" << std::endl;
		return 1;
	} else {
		std::cout << "Successful send Packet seqNum" <<
		Packet.seqNum << "/" << "ackNum" << 
		Packet.ackNum << std::endl;
	} 
	int status = sendto(sockfd, msg, len, flags, to, tolen);
	memset(&Packet, 0, sizeof(Packet));
	return status;
}

int udprecv(int sockfd, void *buf, int len, unsigned int flags,
				struct sockaddr *from, socklen_t *fromlen,
				float Pl, float Pc) {
	
	memset(&Packet, 0, sizeof(Packet));
	int status = recvfrom(sockfd, buf, len, flags, from, fromlen);
	return status;
}