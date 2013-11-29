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
	if (argc < 2) {
		cerr << "Usage: " << argv[0] << 
			" <sender_portnumber> " << endl;
		return 0;
	}

	// the socket addr container
	struct sockaddr_in server_addr, cli_addr;
   	socklen_t slen = sizeof(cli_addr);

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
		bytes_received = recvfrom(fd, (void*)&Packet, packetSize, 
        				  0, (struct sockaddr*)&cli_addr, &slen);
        if (bytes_received == -1)
            cerr << "recvfrom() failed" << endl;
	
		cout << "Received packet from: " << 
			inet_ntoa(cli_addr.sin_addr) <<":";
		cout << ntohs(cli_addr.sin_port) << endl;
		cout << "Data: " << Packet.payload << endl;

		//readFile
		cout << "reading file" << endl;
		//Packet.payload[bytes_received] = '\0';	
		char buffer[100];
		int count = 0;

		//get filename;
		string filename(Packet.payload);
		cout << filename << endl;
		//read the file from the sender path
		ifstream fin(filename.c_str(),ios::binary);
			
		//file is not exist
		if(!fin) {
			char err_msg [] = "the file you request does not exist";
			//write(fd, err_msg, strlen(err_msg));
			sendto(fd, err_msg, strlen(err_msg), 0,
			   (struct sockaddr*)&cli_addr, slen);
			cout << "File open error!" << endl;
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
		fsize = size;
		fin1.seekg(beg); // resets stream pointer to the beginning
		/************ finish to caculate the fliesize*********/

		// determine the number of packets to be sent
		int numPackets = fsize / DATASIZE;
		if (fsize % DATASIZE != 0)
			numPackets++;
	
		int i = 0;
		for(; i < numPackets; ++i)
		{
			fin.read(Packet.payload, DATASIZE);
			count = fin.gcount();
			sendto(fd, (void*)&Packet, count + headSize, 0,
			  (struct sockaddr*)&cli_addr, slen);
			cout << "sending data amount: " << count << endl;
		}
		
		cout << "server send " << i << " packets to client" << endl;	
	
		fin.close();
		fin1.close();
    }

	cout << "Closing socket..." << endl;
	close(fd);
}