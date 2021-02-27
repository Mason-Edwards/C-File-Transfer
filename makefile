all: server client
LDFLAGS = -pthread

server: server.c
client: client.c


clean:
	-rm -f *.o server client
