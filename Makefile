CC=g++

all: echos echo

echos: server.cpp sbcp.cpp
	$(CC) -o echos server.cpp sbcp.cpp -lpthread
echo: client.cpp sbcp.cpp
	$(CC) -o echo client.cpp sbcp.cpp -lpthread

.PHONY: clean

clean:
	rm -f echos echo