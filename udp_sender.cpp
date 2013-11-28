#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <fstream>
#include <string>
#include <stdlib.h>
#include "packet.h"
using namespace std;

int main(int argc, char* argv[])
{
	if (argc < 2) {
		cerr << "Usage: " << argv[0] << 
			" <sender_portnumber> " << endl;
		return 0;
	}

	// the socket addr container
	struct sockaddr_in myaddr;

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
	long portnum = strtol(argv[1], NULL, 10);

	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htonl(portnum);

	if (bind(fd, (struct sockaddr *)&myaddr, 
			sizeof(myaddr)) < 0) {
		cerr << "Failed to bind to " << portnum << endl;
		return 0;
	} else {
		cout << "Socket bound to port " << portnum << "!"
			 << endl;
	}


	//Listening for incoming connections...
	cout<<"Listening for incoming connections..."<<endl;

	int status=listen(fd,5);
	if(status==-1) cout<<"Listen error"<<endl;

	while(true)
	{
		//Accepting connections...
		cout<<"Waiting for connections..."<<endl;
		int new_socketfd;	//for each connection creat a socketfd to handle the request.
		struct sockaddr_storage their_addr;  //store the incoming connections of the client IP address and port number.
		socklen_t addr_size = sizeof(their_addr);

		new_socketfd = accept(fd, (struct sockaddr *)&their_addr, &addr_size);

		if(new_socketfd==-1)
		{
			cout<<"accept error"<<endl;
		}
		else
		{
			cout<<"Successfully accepted the connection, using the new socketfd: "<<new_socketfd<<"\r\n"<<endl;
		}



		//Waiting to receive data...
		cout << "Waiting to receive data..."  << endl;

		/*
		*receive the first packet(request filename) from client- send ACK/NCK 
		*send packets
		*/
		ssize_t bytes_received = recv(new_socketfd, (void*)&Packet,packetSize, 0);
		if (bytes_received != -1)
		{
			

			Packet.payload[bytes_received] = '\0';

			//get filename;
			string filename(Packet.payload);

			//read the file from the sender path
			ifstream fin(filename.c_str(),ios::binary);
			
			//file is not exist
			if(!fin){
				char err_msg [] ="the file you request does not exist";
				write(fd, err_msg, strlen(err_msg));
				cout<<"File open error!\n";
				//send(NAK);
			}

			/************ start to caculate the fliesize*********/
			streampos size, beg;
			int fsize;
			ifstream fin1(filename.c_str(),ios::binary);

  			// uses a buffer to assess the file size
			beg = fin1.tellg();
			fin1.seekg(0, std::ios::end);
			size = fin1.tellg() - beg;
			fsize=size;
			fin1.seekg(beg); // resets the stream pointer to the beginning
			
			/************ finish to caculate the fliesize*********/
			
			/*send(ACK); Tell the client that server successful find the file 
			* and the filename is right
			*/

			//decide how much pkts needed
			int packetNum = fsize / DATASIZE;
			if (fsize % DATASIZE!=0)
			{
				packetNum += 1;
			}


			//send the file from the server
			int count = 0;
			for (int i = 0; i < packetNum; ++i)
			{
				fin.read(Packet.payload,DATASIZE);
				count = fin.gcount();
				//add #seq & checksum;
				Packet.seq=i;
				Packet.checksum=i^2;
				//send packet
				write(new_socketfd, (void*)&Packet,count + headSize);

			}

			fin.close();
			fin1.close();

		}
		else
		{
			//send(NCK);
		}


		//Stopping the server...
		cout<<"Stopping the server...\r\n"<<endl;

		close(new_socketfd);
		
	}


	cout << "Closing socket..." << endl;
	close(fd);
	
	// testtest
}