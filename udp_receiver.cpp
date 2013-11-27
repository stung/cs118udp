#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <stdio.h>
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
	struct sockaddr_in myaddr;

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
	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htonl(0);

	if (bind(fd, (struct sockaddr *)&myaddr, 
			sizeof(myaddr)) < 0) {
		cerr << "Failed to bind to " << "portnum" << endl;
		return 0;
	} else {
		cout << "Socket bound to port " << "portnum" << "!"
			 << endl;
	}
	
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

	cout << "Closing socket..." << endl;
	close(fd);
}