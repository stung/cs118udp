

const int DATASIZE= 1024;

struct Packet{
	int seq;
	int checksum;
	char[DATASIZE] payload;

}

const int headSize=2*sizeof(int);
const int packetSize=headSize+DATASIZE;