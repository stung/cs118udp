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
	// strncpy(host, argv[1], sizeof(host)); 
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
 	strncpy(Packet.payload, filename, sizeof(Packet.payload));
	Packet.type = FILE_TRANSFER_REQUEST;
	ssize_t bytes_received;
	int exp_pktNum = 0;
	int pkt_ackNum = -1;

	if (udpsend(fd, (void*)&Packet, strlen(filename) + headSize, 0, 
		  (struct sockaddr*)&serv_addr, slen, Pl, Pc) != -1)
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
				cout << "Max Seq Num is: " << maxSeqNum << endl;	
				cout << "Creating file" << endl;

				string received_file = filename;
				ofstream newfile(received_file.c_str());

				if (newfile.is_open()) {
					while(1) {
						bytes_received = udprecv(fd, (void*)&Packet,
				 			packetSize, 0, (struct sockaddr*)&serv_addr,
							&slen, Pl, Pc);
						if (bytes_received != -1){
							//file transfer complete
							if (Packet.type == FILE_TRANSFER_COMPLETE)
							{
								cout << Packet.payload << endl;
								
								//send ACK
								Packet.type = TRANSFER_COMPLETE_ACK;
                                if (udpsend(fd,(void*)&Packet, headSize, 0, 
		  								(struct sockaddr*)&serv_addr, slen, Pl, Pc) != -1 )
                                    cout << "sending transfer complete ACK" << endl;
								break;
							}
							//file corruption
							else if (Packet.type == FILE_CORRUPTION) {
								//inform packet corruption
								cout << "Corruption detected in packet" << Packet.seqNum << endl;
								
								//send ACK when the first unacked pkt in the CW was corrupted
								if (exp_pktNum == Packet.seqNum)
								{
									if (Packet.seqNum == 0) {
										Packet.seqNum = maxSeqNum;
									}
									Packet.ackNum = Packet.seqNum - 1;
									Packet.type = ACK;
                                	memset(&Packet.payload, 0, sizeof(Packet.payload));
                                	if (udpsend(fd,(void*)&Packet,headSize,0, 
		  									(struct sockaddr*)&serv_addr, slen, Pl, Pc) != -1 )
                               	 	{
                                    	cout << "sending ACK" << Packet.ackNum << endl;
                                	}
								} /*else {

								}*/
							} else if (Packet.type == FILE_DATA) {
								//get the expected pkt
								cout << "Current seqNum" << Packet.seqNum << endl;
								if (exp_pktNum == Packet.seqNum) {
									//writing data
									newfile.write(Packet.payload, 
     								bytes_received - headSize);
									cout << "writing " << bytes_received - headSize
									<< " bytes into file" << endl;
									exp_pktNum++;
									pkt_ackNum++;
									pkt_ackNum = pkt_ackNum % maxSeqNum;
									exp_pktNum = exp_pktNum % maxSeqNum;
									memset(&Packet.payload, 0, sizeof(Packet.payload));
								} else {
									//inform packet loss
									cout << "Packet" << exp_pktNum << " dropped" <<endl;
								}

								//send ACK 
								Packet.type = ACK;
								Packet.ackNum = pkt_ackNum;
								memset(&Packet.payload, 0, sizeof(Packet.payload));
                                if (udpsend(fd, (void*)&Packet, headSize, 0, 
		  								(struct sockaddr*)&serv_addr, slen, Pl, Pc) != -1 )
                                {
                                    cout << "sending ACK" << Packet.ackNum << endl;
                                }
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