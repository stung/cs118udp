#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <fstream>
#include <string>
#include <stdlib.h>
#include "packet.h"
#define BUFLEN 512
using namespace std;

int main(int argc, char* argv[])
{
	if (argc < 5) {
		cerr << "Usage: " << argv[0] << 
			" <sender_portnumber> " << "CWnd " << 
			"Pl " << "Pc " << endl;
		return 0;
	}

	// the socket addr container
	struct sockaddr_in server_addr, cli_addr;
   	socklen_t slen = sizeof(cli_addr);

   	float Pl = (float)strtod(argv[3], NULL);
   	float Pc = (float)strtod(argv[4], NULL);

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
	htonl converts long to network rep (address/port #)
	*/

	short portnum = strtol(argv[1], NULL, 10);

	memset((char *)&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(portnum);

	if (bind(fd, (struct sockaddr *)&server_addr,
			sizeof(server_addr)) < 0) {
		cerr << "Failed to bind to " << portnum << endl;
		return 0;
	} else {
		cout << "Socket bound to port " << portnum << "!"
			 << endl;
	}

    while(1)
    {
    	cout << "Waiting for data..." << endl;
		ssize_t bytes_received;
		bytes_received = udprecv(fd, (void*)&Packet, packetSize, 
        				  0, (struct sockaddr*)&cli_addr, &slen, Pl, Pc);
        if (bytes_received != -1)
        {
			if (Packet.type == FILE_TRANSFER_REQUEST)
			{

				string filename(Packet.payload);
				int count = 0;
				cout << "Receiving " << filename << " request from: " << 
					inet_ntoa(cli_addr.sin_addr) <<":";
				cout << ntohs(cli_addr.sin_port) << endl;

				//read the file from the sender path
				ifstream fin(filename.c_str(),ios::binary);

				//file does not exist
				if(!fin) {
					Packet.type = FILE_NOTEXIST_ERROR;
					char err_msg [] = "the file you requested does not exist";
					memset(&Packet.payload, 0, sizeof(Packet.payload));
					strncpy(Packet.payload, err_msg, strlen(err_msg));
					if(udpsend(fd, (void*)&Packet, strlen(err_msg) + headSize, 0,
			 		 	(struct sockaddr*)&cli_addr, slen, Pl, Pc) != -1)
						cout << "File open error!" << endl;
				} else {					//file transfer
					int CWnd = strtol(argv[2], NULL, 10);
					int CW_unused = CWnd;
					int expect_ackNum = 0;
					int pkt_seqNum = -1;

					// determine the max number of seq #s
					int pktsPerWnd = CWnd / DATASIZE;
					if (CWnd % DATASIZE != 0)
						pktsPerWnd++;
					int maxSeqNum = pktsPerWnd * 2;	

					int * tran_DataSize = new int[maxSeqNum];
					cout << "Max seq num is " << maxSeqNum << endl;

					Packet.type = FILE_EXISTS;
					char begin_msg [] = "begin to read from file";
					Packet.maxSeqNum = maxSeqNum;
					memset(&Packet.payload, 0, sizeof(Packet.payload));
					strncpy(Packet.payload, begin_msg, strlen(begin_msg));
					if(udpsend(fd, (void*)&Packet, strlen(begin_msg) + headSize, 0,
			 		 	(struct sockaddr*)&cli_addr, slen, Pl, Pc) != -1)
						cout << "File transfer initiated!" << endl;
					cout << "Reading file" << endl;
					/************ start to caculate the fliesize*********/
					streampos size, beg;
					int fsize;
					ifstream fin_sizepos(filename.c_str(),ios::binary);

	  				// uses a buffer to assess the file size
					beg = fin_sizepos.tellg();
					fin_sizepos.seekg(0, std::ios::end);
					size = fin_sizepos.tellg() - beg;
					fsize = size;
					fin_sizepos.seekg(beg); // resets stream pointer to the beginning
					/************ finish to caculate the filesize*********/


					while(!fin.eof()) {
						while(CW_unused > 0) {
							pkt_seqNum++;
							Packet.seqNum = pkt_seqNum % maxSeqNum;
							Packet.type = FILE_DATA;
							if ( CW_unused < DATASIZE )
							{
								fin.read(Packet.payload, CW_unused);
								tran_DataSize[Packet.seqNum] = CW_unused;
								CW_unused = 0;
							} else {
								fin.read(Packet.payload, DATASIZE);
								tran_DataSize[Packet.seqNum] = DATASIZE;
								CW_unused -= DATASIZE;  
							}
							
							count = fin.gcount();
							udpsend(fd, (void*)&Packet, count + headSize, 0,
			 	 				(struct sockaddr*)&cli_addr, slen, Pl, Pc);
							cout << "sending data amount: " << count << endl;
							cout << "Sending Pkt SeqNum" << Packet.seqNum << endl;
						}

						//receive ack , need to use non-block recvfrom
						bytes_received = udprecv(fd, (void*)&Packet, packetSize, 
        				  0, (struct sockaddr*)&cli_addr, &slen, Pl, Pc);
						cout << "Current ack received: " << Packet.ackNum << endl;
						if (bytes_received != -1) {
							if (Packet.type == ACK)
							{
								//successfully receive the right ack, move the CW
								if (Packet.ackNum == expect_ackNum)
								{
									CW_unused += tran_DataSize[Packet.ackNum];
									expect_ackNum++;
									expect_ackNum = expect_ackNum % maxSeqNum;
									// restart the timer
									// continue;
								}
							}

							/*corruption or ack lost or not receive the right ackNum
							* waiting for timeout
							* modify the file pointer
							* reset some parameter to implement the file retransfer
							*/
						}
					}

					fin.close();
					fin_sizepos.close();
					//file transfer complete
					while(1) {
						Packet.type = FILE_TRANSFER_COMPLETE;
						char msg [] = "The file transfer is completed!";
						memset(&Packet.payload, 0, sizeof(Packet.payload));
						strncpy(Packet.payload, msg, strlen(msg));

						if(udpsend(fd, (void*)&Packet, strlen(msg) + headSize, 0,
			 		 		(struct sockaddr*)&cli_addr, slen, Pl, Pc) != -1 ) {
							//check ack
							bytes_received = udprecv(fd, (void*)&Packet,
					 			packetSize, 0, (struct sockaddr*)&server_addr,
								&slen, Pl, Pc);

							if (bytes_received != -1) {
								//file transfer complete
								if (Packet.type == TRANSFER_COMPLETE_ACK)
								{
									cout << "The file transfer is completed!" << endl;
									break;
								}
							}
						}
					}
				}
			}
        } else {
        	cerr << "recvfrom() failed" << endl;
        }
    }

	cout << "Closing socket..." << endl;
	close(fd);
}