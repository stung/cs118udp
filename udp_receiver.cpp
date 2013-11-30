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
	if (argc < 4) {
		cerr << "Usage: " << argv[0] << 
			" <sender_hostname> " << 
			"<sender_portnumber> " <<
			"<filename>" << endl;
		return 0;
	}

	// the socket addr container
	//struct sockaddr_in myaddr;
	struct sockaddr_in serv_addr;
	int i;
	socklen_t slen = sizeof(serv_addr);
	char buf[BUFLEN];

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

	cout << "Binding socket..." << endl;

	/* 
	Bind to arbitrary return addr (client side)
	Client will only send responses
	INADDR_ANY: any IP addr
	0: OS will select the port number
	htonl converts long to network rep (address)
	htons converts short to network rep (port #)
	*/

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

	if (sendto(fd, (void*)&Packet, strlen(filename) + headSize, 0, 
		  (struct sockaddr*)&serv_addr, slen) != -1)
	{
		//receive the packet to know if the file is exsit
		bytes_received = recvfrom(fd, (void*)&Packet,
				 packetSize, 0, (struct sockaddr*)&serv_addr,
				 &slen);
		if (bytes_received != -1)
		{
			//cannot find file  
			if (Packet.type == FILE_NOTEXIST_ERROR)
			{
				cerr << Packet.payload << endl;
				return 0;
			}
			else
			{
				cout << "Creating file" << endl;

				string received_file = filename;
				ofstream newfile(received_file.c_str());

				if (newfile.is_open()) {

					while(true){
						bytes_received = recvfrom(fd, (void*)&Packet,
				 			packetSize, 0, (struct sockaddr*)&serv_addr,
							&slen);
						if (bytes_received != -1){
							//file transfer complete
							if (Packet.type == FILE_TRANSFER_COMPLETE)
							{
								cout << Packet.payload << endl;
								
								//send ACK
								Packet.type = TRANSFER_COMPLETE_ACK;
                                if ( sendto(fd,(void*)&Packet, headSize, 0, 
		  								(struct sockaddr*)&serv_addr, slen) != -1 )
                                {
                                    cout << "sending ACK" << Packet.ackNum << endl;
                                }

								break;
							}
							//file corruption
							else if (Packet.type == FILE_CORRUPTION) {
								cout << Packet.payload << endl;
								
								//send ACK
								Packet.type = ACK;
                                if ( sendto(fd,(void*)&Packet,headSize,0, 
		  								(struct sockaddr*)&serv_addr, slen) != -1 )
                                {
                                    cout << "sending ACK" << Packet.ackNum << endl;
                                }
							} else if (Packet.type == FILE_DATA){
								//get the expected pkt
								if (exp_pktNum == Packet.seqNum) {
									//writing data
									newfile.write(Packet.payload, 
     								bytes_received - headSize);
									cout << "writing " << bytes_received - headSize
									<< " bytes into file" << endl;
									exp_pktNum++;
									Packet.ackNum++;
								} else {
									//inform packet loss
									cout << "Packet" << exp_pktNum << "lost" <<endl;
								}

								//send ACK 
								Packet.type = ACK;
                                if (sendto(fd, (void*)&Packet, headSize, 0, 
		  								(struct sockaddr*)&serv_addr, slen) != -1 )
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
	//file requset sendto() fail
	else{

		 cerr << "sendto() fail" << endl;
	}
	cout << "Closing socket..." << endl;
	close(fd);
}