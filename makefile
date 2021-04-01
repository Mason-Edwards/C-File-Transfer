#CFLAGS =
LDFLAGS = -lpthread
OBJS = utils.c

all: $(OBJS)
	cc server.c $(OBJS) -o server $(LDFLAGS)
	cc client.c $(OBJS) -o client $(LDFLAGS)
	mv client test/

debug: $(OBJS)
	cc server.c $(OBJS) -o server -g $(LDFLAGS)
	cc client.c $(OBJS) -o client -g $(LDFLAGS)
	mv client test/

clean:
	rm -f *.o server client
	rm -f test/client
	rm -f file
