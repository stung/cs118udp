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

void err(char *str)
{
    cerr << str << endl;
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
    int i, slen=sizeof(serv_addr);
    char buf[BUFLEN];

	// the host entity container
	struct hostent *hp;
	char* host = argv[1];
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
	serv_addr.sin_port = htons(5530);

	/*
	if (bind(fd, (struct sockaddr *)&myaddr, 
			sizeof(myaddr)) < 0) {
		cerr << "Failed to bind to " << "portnum" << endl;
		return 0;
	} else {
		cout << "Socket bound to port " << "portnum" << "!"
			 << endl;
	}
	*/

	// cout << "IP address assigned: " << 

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
    char * ipaddr = (char*) hp -> h_addr_list[0];
    cout << ipaddr << endl;

    if (inet_aton("127.0.0.1", &serv_addr.sin_addr)==0)
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }


	//Waiting to receive data...
	// cout << "Waiting to receive data..."  << endl;

	/*send file request
	* the first packet is only include file name and header info
	* once receive file data from server, write it into a file
	* send ack and nak
	*/

    while(1)
    {
        printf("\nEnter data to send(Type exit and press enter to exit) : ");
        scanf("%[^\n]",buf);
        getchar();
        if(strcmp(buf,"exit") == 0)
          exit(0);
 
        if (sendto(fd, buf, BUFLEN, 0, (struct sockaddr*)&serv_addr, slen)==-1)
            err("sendto()");
    }






	cout << "Closing socket..." << endl;
	close(fd);
}