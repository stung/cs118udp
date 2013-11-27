CC=g++

all: receiver.o sender.o

receiver.o: udp_receiver.cpp
	$(CC) udp_receiver.cpp -o receiver.o

sender.o: udp_sender.cpp
	$(CC) udp_sender.cpp -o sender.o

clean:
	rm -rf *.o