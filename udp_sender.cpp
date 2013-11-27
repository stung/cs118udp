#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <fstream>
#include <string>
#include <stdlib.h>
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

	status=listen(socketfd,5);
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
		cout << "Waiting to receive data..."  << std::endl;

		/*
		*receive the first packet(request filename) from client- send ACK/NCK 
		*send packets
		*/
		ssize_t bytes_received;
		char incomming_data_buffer[packetSize];
		if (bytes_received = recv(new_socketfd, (void*)&packet,packetSize, 0);
!=-1)
		{
			send(ACK);

			packet.payload[bytes_received] = '\0';

			//get filename;
			string filename(pakcket.payload);

			//read the file from the sender path
			//caculate the fliesize
			//decide how much pkts needed
			//send the file from the server

		}
		else
		{
			send(NCK);
		}










		//Stopping the server...
		cout<<"Stopping the server...\r\n"<<endl;

		close(new_socketfd);
		
	}


	cout << "Closing socket..." << endl;
	close(fd);
	
	// testtest
}