#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include "packet.h"
#define BUFLEN 512
#define DEBUG 0
using namespace std;

// prints the IP address in dotted decimal
void paddr(unsigned char a[])
{
	cout << (int)a[0] << "." << (int)a[1] << "." <<
			(int)a[2] << "." << (int)a[3] << endl;
	// printf("%d.%d.%d.%d\n", a[0], a[1], a[2], a[3]);
}

int main(int argc, char* argv[])
{
	if (argc < 6) {
		cerr << "Usage: " << argv[0] << 
			" <sender_hostname> " << 
			"<sender_portnumber> " <<
			"<filename> " << "Pl " << "Pc " << endl;
		return 0;
	}

	// the socket addr container
	//struct sockaddr_in myaddr;
	struct sockaddr_in serv_addr;
	int i;
	socklen_t slen = sizeof(serv_addr);
	char buf[BUFLEN];
	float Pl = (float)strtod(argv[4], NULL);
	float Pc = (float)strtod(argv[5], NULL);
	// initialize random seed
	srand(time(0));

	// the host entity container
	struct hostent *hp;
	struct in_addr **addr_list;

	char* host = argv[1];
	short portnum = strtol(argv[2], NULL, 10);
	cout << "Creating socket..." << endl;

	// Create a socket
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		cerr << "Socket could not be created" << endl;
		return 0;
	}

	memset((char *)&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portnum);

	cout << "Locating " << host << "..." << endl;
	hp = gethostbyname(host);
	if (!hp) {
		cerr << "Could not resolve " << host << endl;
		return 0;
	}

	for (int i=0; hp -> h_addr_list[i] != 0; i++) {
		cout << "Hostname resolved to: ";
		paddr((unsigned char*) hp -> h_addr_list[i]);
	}

	//assign the serv_addr from the argv[1]
   	addr_list = (struct in_addr **)hp->h_addr_list;

	if (inet_aton(inet_ntoa(*addr_list[0]), &serv_addr.sin_addr)
			 == 0)
	{
  	  cerr << "inet_aton() failed" << endl;
  	  return 0;
  	}
	
	/*send file request
	* the first packet is only include file name and header info
	* once receive file data from server, write it into a file
	* send ack and nak
	*/

	char* filename = argv[3];
	memset(&Packet.payload, 0, sizeof(Packet.payload));
 	strncpy(Packet.payload, filename, strlen(filename));
	Packet.type = FILE_TRANSFER_REQUEST;
	ssize_t bytes_received;
	int exp_pktNum = 0;
	int pkt_ackNum = -1;
	int status = 0;
	int ACKNumB = 0;

	if (udpsend(fd, (void*)&Packet, strlen(filename) + headSize, 0, 
		  (struct sockaddr*)&serv_addr, slen, 0, 0) != -1)
	{
		//receive the packet to know if the file is exsit
		bytes_received = udprecv(fd, (void*)&Packet,
				 packetSize, 0, (struct sockaddr*)&serv_addr,
				 &slen, Pl, Pc);
		if (bytes_received != -1)
		{
			//cannot find file  
			if (Packet.type == FILE_NOTEXIST_ERROR)
			{
				cerr << Packet.payload << endl;
				return 0;
			} else {
				int maxSeqNum = Packet.maxSeqNum;
				if (DEBUG) {
					cout << "Max Seq Num is: " << maxSeqNum << endl;	
				}
				cout << "Creating file" << endl;

				string received_file = filename;
				ofstream newfile(received_file.c_str());

				if (newfile.is_open()) {
					while(1) {
						bytes_received = udprecv(fd, (void*)&Packet,
			 			packetSize, 0, (struct sockaddr*)&serv_addr,
						&slen, Pl, Pc);
						if (bytes_received != -1) {
							//file transfer complete
							if (Packet.type == FILE_TRANSFER_COMPLETE) {
								cout << Packet.payload << endl;
								
								//send ACK
								Packet.type = TRANSFER_COMPLETE_ACK;
                            	memset(&Packet.payload, 0, sizeof(Packet.payload));
                            	Packet.seqNum = -2; // invalid seqNum
                            	Packet.ackNum = -2; // invalid ackNum
								Packet.byteSeqNum = -2; // invalid seqNum
								Packet.byteAckNum = -2; // invalid ackNum
                            	status = udpsend(fd,(void*)&Packet, headSize, 0, 
		  								(struct sockaddr*)&serv_addr, slen, 0, 0);
                                if (status != -1)
                                    cout << "Sending transfer complete ACK" << endl;
								break;
							} else if (Packet.type == FILE_CORRUPTION) { //file corruption
								//inform packet corruption
								cout << "Corruption detected in packet " <<
									Packet.byteSeqNum << endl;
								cout << "CORRUPTION DETECTED, DROPPING PACKET----------------" << 
									endl;
								cout << "ACKNUM IS AT BYTE " << ACKNumB << endl << endl; 
								//send ACK 
								Packet.type = ACK;
								Packet.ackNum = pkt_ackNum;
								Packet.seqNum = -2; // invalid seqNum
								Packet.byteSeqNum = -2; // invalid seqNum
								memset(&Packet.payload, 0, sizeof(Packet.payload));

								status = udpsend(fd, (void*)&Packet, headSize, 0, 
		  								(struct sockaddr*)&serv_addr, slen, Pl, Pc);
                                if (status != -1)
                                {
                                    cout << "sending ACK" << ACKNumB << endl << endl;
                                }
							} else if (Packet.type == FILE_DATA) {
								//get the expected pkt
								if (DEBUG) {
									cout << "Current seqNum" << Packet.seqNum << endl;
								}
								if (exp_pktNum == Packet.seqNum) {
									//writing data
									newfile.write(Packet.payload, bytes_received - headSize);
									cout << "writing " << bytes_received - headSize
									<< " bytes into file" << endl << endl;
									exp_pktNum++;
									pkt_ackNum++;
									pkt_ackNum = pkt_ackNum % maxSeqNum;
									exp_pktNum = exp_pktNum % maxSeqNum;
									memset(&Packet.payload, 0, sizeof(Packet.payload));
									if (DEBUG) {
										cout << "Packet" << Packet.byteSeqNum << " written, " <<
										"expecting packet" << exp_pktNum << " next" << endl;
									}
									cout << "Packet" << Packet.byteSeqNum << " written!" << endl;
									ACKNumB = Packet.byteSeqNum;
									cout << "ACKNUM IS AT BYTE " << ACKNumB << endl << endl; 
								} else {
									//inform packet loss
									if (DEBUG) {
										cout << "Expected packet" << exp_pktNum << 
										", packet" << Packet.seqNum << " dropped" << endl;
									}
									cout << "Received packet" << Packet.byteSeqNum << 
										", out of order!" << endl;
									cout << "ACKNUM IS AT BYTE " << ACKNumB << endl << endl; 
								}

								//send ACK 
								Packet.type = ACK;
								Packet.ackNum = pkt_ackNum;
								Packet.byteAckNum = ACKNumB;
								Packet.seqNum = -2; // invalid seqNum
								Packet.byteSeqNum = -2; // invalid seqNum
								memset(&Packet.payload, 0, sizeof(Packet.payload));

								status = udpsend(fd, (void*)&Packet, headSize, 0, 
		  								(struct sockaddr*)&serv_addr, slen, Pl, Pc);
                                if (status != -1)
                                {
                                    cout << "sending ACK" << ACKNumB << endl << endl;
                                }
								
								cout << "FILE DATA PACKET PROCESSED" << 
									"--------------------------------" << endl << endl;
							} else if (Packet.type == TRANSFER_ABORT) {
								cout << "CONNECTION TOO UNSTABLE" << endl;
								cout << "FILE TRANSFER ABORTED" << endl;
								cout << "-------------------------------------------" << endl;
								return 0;
							}
						}  
					}
				}
				newfile.close();	
			}
		} else {
			cerr << "recvfrom() file ack fail" << endl;
		}
	}
	//file request sendto() fail
	else {
		 cerr << "sendto() fail" << endl;
	}
	cout << "Closing socket..." << endl;
	close(fd);
}