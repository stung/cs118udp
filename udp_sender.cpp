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
#define DEBUG 0
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
	// initialize random seed	
	srand(time(0));

   	float Pl = (float)strtod(argv[3], NULL);
   	float Pc = (float)strtod(argv[4], NULL);
   	int sock_status;

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

	short portnum = (short)strtol(argv[1], NULL, 10);

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

    while(1) {
    	cout << "Waiting for data..." << endl;
		ssize_t bytes_received;
		bytes_received = udprecv(fd, (void*)&Packet, packetSize, 
        				  0, (struct sockaddr*)&cli_addr, &slen, 0, 0);
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
			 		 	(struct sockaddr*)&cli_addr, slen, 0, 0) != -1)
						cout << "File open error!" << endl;
				} else { //file transfer
					int CWnd = strtol(argv[2], NULL, 10);
					int CW_unused = CWnd;
					int expect_ackNum = 0;
					int pkt_seqNum = 0;
					streampos cumAckPointer;

					// determine the max number of seq #s
					int pktsPerWnd = CWnd / DATASIZE;
					if (CWnd % DATASIZE != 0)
						pktsPerWnd++;
					int maxSeqNum = pktsPerWnd * 2;	

					int * tran_DataSize = new int[maxSeqNum];
					if (DEBUG) {
						cout << "Max seq num is " << maxSeqNum << endl;
					}

					Packet.type = FILE_EXISTS;
					char begin_msg [] = "begin to read from file";
					Packet.maxSeqNum = maxSeqNum;
					memset(&Packet.payload, 0, sizeof(Packet.payload));
					strncpy(Packet.payload, begin_msg, strlen(begin_msg));
					if(udpsend(fd, (void*)&Packet, strlen(begin_msg) + headSize, 0,
			 		 	(struct sockaddr*)&cli_addr, slen, 0, 0) != -1)
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

					// Calculating the packet size for each seq num
					for(int x = 0; x < maxSeqNum; x++) {
						if ((x != (pktsPerWnd - 1)) &&
							(x != (maxSeqNum - 1))) {
							tran_DataSize[x] = DATASIZE;
							if (DEBUG) {
								cout << "x is " << tran_DataSize[x] << endl;
							}
						} else {
							tran_DataSize[x] = CWnd % DATASIZE;
							if (DEBUG) {
								cout << "CWnd is " << CWnd << endl;
								cout << "x is small " << tran_DataSize[x] << endl;
							}
						}
					}

				   	struct timeval timeout;
				   	timeout.tv_sec = 0;
					timeout.tv_usec = 20000; // in microseconds
					while(cumAckPointer < fsize) {
					   	// select setup
					   	int select_status;
					   	fd_set readset;
					   	FD_ZERO(&readset);
					   	FD_SET(fd, &readset);

						while(CW_unused > 0) {
							memset(&Packet.payload, 0, sizeof(Packet.payload));
							Packet.type = FILE_DATA;
							Packet.seqNum = pkt_seqNum;

							if ( CW_unused < DATASIZE )
							{
								fin.read(Packet.payload, tran_DataSize[Packet.seqNum]);
								CW_unused = 0;
								if (DEBUG) {
									cout << "CWunFile pointer is " << fin.tellg() << endl;
									cout << "cumAckPointer is " << cumAckPointer << endl;
								}
							} else {
								fin.read(Packet.payload, tran_DataSize[Packet.seqNum]);
								CW_unused -= tran_DataSize[Packet.seqNum];
								if (DEBUG) {
									cout << "DATAFile pointer is " << fin.tellg() << endl;
									cout << "cumAckPointer is " << cumAckPointer << endl;
								}
							}
							
							count = fin.gcount();
							if (count > 0) {
								Packet.ackNum = -2; // invalid ackNum
								Packet.byteAckNum = -2; // invalid ackNum
								pkt_seqNum++;
								pkt_seqNum = pkt_seqNum % maxSeqNum;
								if (DEBUG) {
									cout << "Write Payload is: -------------------" <<
									"-----------------------" << endl << Packet.payload << 
									endl << "-------------------------------------------------" << endl;
								}
								Packet.byteSeqNum = cumAckPointer;
								udpsend(fd, (void*)&Packet, count + headSize, 0,
				 	 				(struct sockaddr*)&cli_addr, slen, Pl, Pc);
								if (DEBUG) {
									cout << "Sending Pkt SeqNum" << Packet.seqNum << endl << endl;
								}
								cout << "Sending data amount: " << count << endl;
								cout << "SEQNUM IS AT BYTE " << fin.tellg() << endl;
								cout << "ExpACKNum is at byte " << cumAckPointer << endl << endl;;
							} else {
								cout << "No more characters to read in the file" << endl << endl;
							}
						}

						//receive ack 
						// need to use non-block recvfrom
						select_status = select(fd + 1, &readset, NULL, NULL, &timeout);
						if (FD_ISSET(fd, &readset)) {
							bytes_received = udprecv(fd, (void*)&Packet, packetSize, 
	        				  0, (struct sockaddr*)&cli_addr, &slen, Pl, Pc);
							if (DEBUG) {
								cout << "Current ACK received: " << Packet.ackNum << endl;
							}
							if (Packet.type == ACK) {
								//successfully receive the right ack, move the CW
								if (Packet.ackNum == expect_ackNum) {
									cout << "ACK" << Packet.byteAckNum << " received" << endl;
									if (!fin.eof()) {
										CW_unused += tran_DataSize[Packet.ackNum];
									}
									expect_ackNum++;
									expect_ackNum = expect_ackNum % maxSeqNum;
									cumAckPointer += tran_DataSize[Packet.ackNum];
									timeout.tv_usec = 20000;
									// restart the timer
									// continue;
								} else if ((Packet.ackNum < (expect_ackNum + pktsPerWnd)) &&
                                            (Packet.ackNum > (expect_ackNum))) {
                                    // Looking for in-bounds ACK
                                    if (DEBUG) {
	                                    cout << "ACK" << Packet.ackNum << " received," <<
	                                        " expected packet" << expect_ackNum <<
	                                        ", still within bounds" << endl << endl;
                                    }
	                                    cout << "ACK" << Packet.byteAckNum << " received" << endl << endl;
                                    if (cumAckPointer < fsize) {
                                        for (int i = expect_ackNum; i <= Packet.ackNum; i++) {
                                            if (cumAckPointer < fsize) {
                                                cumAckPointer += tran_DataSize[i];
	                                            if (DEBUG) {
		                                            cout << "cumAckPointer is " << cumAckPointer << endl;
	                                            }
                                            } else {
                                            	break;
                                            }
                                        }
                                    }
                                    expect_ackNum = Packet.ackNum;
                                    expect_ackNum++;
                                    expect_ackNum = expect_ackNum % maxSeqNum;
                                    timeout.tv_usec = 20000;
                                } else if ((expect_ackNum > pktsPerWnd) &&
	                                        ((Packet.ackNum < (expect_ackNum - pktsPerWnd)) || 
	                                        (Packet.ackNum > (expect_ackNum + pktsPerWnd)))) {
                                    // Looking for in-bounds ACK
                                    if (DEBUG) {
	                                    cout << "ACK" << Packet.ackNum << " received," <<
	                                        " expected packet" << expect_ackNum <<
	                                        ", still within bounds" << endl << endl;
                                    }
	                                    cout << "ACK" << Packet.byteAckNum << " received" << endl << endl;
                                    if (cumAckPointer < fsize) {
                                        if (Packet.ackNum < (expect_ackNum - pktsPerWnd)) {
                                            for (int i = expect_ackNum; i < maxSeqNum; i++) {
                                                if (cumAckPointer < fsize) {
                                                    cumAckPointer += tran_DataSize[i];
	                                                if (DEBUG) {
			                                            cout << "cumAckPointer is " << cumAckPointer << endl;
	                                            	}
                                                } else {
                                                	break;
                                                }
                                            }
                                            for (int j = 0; j <= Packet.ackNum; j++) {
                                                if (cumAckPointer < fsize) {
                                                    cumAckPointer += tran_DataSize[j];
	                                                if (DEBUG) {
		                                                cout << "cumAckPointer is " << cumAckPointer << endl;
	                                            	}
                                                } else {
                                                	break;
                                                }
                                            }
                                        } else {
                                            for (int i = expect_ackNum; i <= Packet.ackNum; i++) {
                                                if (cumAckPointer < fsize) {
                                                    cumAckPointer += tran_DataSize[i];
	                                                if (DEBUG) {
		                                                cout << "cumAckPointer is " << cumAckPointer << endl;
	                                            	}
                                                } else {
                                                	break;
                                                }
                                            }
                                        }
                                    }
                                    expect_ackNum = Packet.ackNum;
                                    expect_ackNum++;
                                    expect_ackNum = expect_ackNum % maxSeqNum;
                                    timeout.tv_usec = 20000;
                                } else {
									// drops all ACKs outside of bounds,
									// even if receiver is "buffered" by cumAck
									if (DEBUG) {
										cout << "Expected ACK" << expect_ackNum << 
											", received ACK" << Packet.ackNum << 
											", out of bounds" << endl << endl;	
									}
										cout << "Received ACK" << Packet.byteAckNum << 
											", out of bounds" << endl << endl;	
								}
								cout << "ACK PACKET PROCESSED--------------------------------" <<
									endl << endl;
							} else if (Packet.type == FILE_CORRUPTION) {
								cout << "Packet ACKNum" << Packet.byteAckNum << 
								" is corrupted!" << endl;
								cout << "CORRUPTION DETECTED, DROPPING PACKET----------------" << 
									endl << endl;
								cout << "SEQNUM IS AT BYTE " << fin.tellg() << endl;
								cout << "ExpACKNum is at byte " << cumAckPointer << endl << endl;;
							}
							
							/*corruption or ack lost or not receive the right ackNum
							* waiting for timeout
							* modify the file pointer
							* reset some parameter to implement the file retransfer
							*/
						} else {
							cout << "****************" << 
								"Sender timed out!****************" << endl;
	
							/*reset the CW_unused to retransmit all packets
							* since the first unacked packet*/
							CW_unused = CWnd;
							
							//reset the seqNum
							if (DEBUG) {
								cout << "Resetting the SEQnum to: " << expect_ackNum << endl;
							}
							pkt_seqNum = expect_ackNum;
							
							//modify the file pointer to the send_base
							if (fin.eof()) {
								fin.clear();
								fin.seekg(0, ios::beg);
								if (DEBUG) {
									cout << "Resetting the EOF bit" << endl;
								}
							}
							fin.seekg(cumAckPointer);
							if (DEBUG) {	
								cout << "cumAckPointer is " << cumAckPointer << endl;
								cout << "ACKRstFile pointer is " << fin.tellg() << endl << endl;
							}
						}
						cout << "SEQNUM IS AT BYTE " << fin.tellg() << endl;
						cout << "ExpACKNum is at byte " << cumAckPointer << endl << endl;;
					}

					fin.close();
					fin_sizepos.close();
					// FILE TRANSFER COMPLETE
					while(1) {
						// select setup
					   	int select_finalstatus;
					   	fd_set finalreadset;
					   	FD_ZERO(&finalreadset);
					   	FD_SET(fd, &finalreadset);
					   	struct timeval finaltimeout;
					   	finaltimeout.tv_sec = 0;
					   	finaltimeout.tv_usec = 50000; // in microseconds

						Packet.type = FILE_TRANSFER_COMPLETE;
						char msg [] = "The file transfer is completed!";
						memset(&Packet.payload, 0, sizeof(Packet.payload));
						strncpy(Packet.payload, msg, strlen(msg));

						Packet.seqNum = -2; // invalid seqNum
						Packet.ackNum = -2; // invalid ackNum
						Packet.byteSeqNum = -2; // invalid seqNum
						Packet.byteAckNum = -2; // invalid ackNum
						sock_status = udpsend(fd, (void*)&Packet, strlen(msg) + headSize,
							 			0, (struct sockaddr*)&cli_addr, slen, Pl, Pc);
						cout << "Waiting for transfer complete ACK" << endl;

						if(sock_status != -1 ) {
							select_finalstatus = select(fd + 1, &finalreadset, NULL, NULL, &finaltimeout);
							if (FD_ISSET(fd, &finalreadset)) {
								//check ack
								bytes_received = udprecv(fd, (void*)&Packet,
							 			packetSize, 0, (struct sockaddr*)&server_addr,
										&slen, Pl, Pc);
								if (bytes_received != -1) {

									if (DEBUG) {
										cout << "Packet" << Packet.seqNum << " received" << endl;
										cout << "Packet type is " << Packet.type << endl;
									}
									//file transfer complete
									if (Packet.type == TRANSFER_COMPLETE_ACK)
									{
										cout << "The file transfer is completed!" << endl;
										break;
									}
								}
							} else {
								cout << "Final transfer complete ACK timeout!" << endl;
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