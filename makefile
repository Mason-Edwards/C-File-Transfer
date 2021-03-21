#CFLAGS =
LDFLAGS = -pthread

#all: server client

all: server.c client.c
	cc server.c -o server $(LDFLAGS)
	cc client.c -o client $(LDFLAGS)
	mv client test/

debug:
	cc server.c -o server -g $(LDFLAGS)
	cc client.c -o client -g $(LDFLAGS)
	mv client test/



clean:
	rm -f *.o server client
	rm -f test/client
	rm -f file
