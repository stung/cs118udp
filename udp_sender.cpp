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

	cout << "Closing socket..." << endl;
	close(fd);
	
	// testtest
}