CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=gnu17
LDFLAGS =

TARGETS = client proc-server thread-server preforked-server reuse-server file-server file-client-tcp

.PHONY: all clean

all: $(TARGETS)

client: client.o err.o common.o

proc-server: proc-server.o err.o common.o

thread-server: thread-server.o err.o common.o
	$(CC) $^ -o $@ -lpthread

preforked-server: preforked-server.o err.o common.o

reuse-server: reuse-server.o err.o common.o

file-server: file-server.o err.o common.o

file-client-tcp: file-client-tcp.o err.o common.o

# To są zależności wygenerowane automatycznie za pomocą polecenia `gcc -MM *.c`.

client.o: client.c err.h common.h
common.o: common.c err.h common.h
err.o: err.c err.h
preforked-server.o: preforked-server.c err.h common.h
proc-server.o: proc-server.c err.h common.h
reuse-server.o: reuse-server.c err.h common.h
thread-server.o: thread-server.c err.h common.h
file-server.o: file-server.c err.h common.h protocol.h
file-client-tcp.o: file-client-tcp.c common.h protocol.h err.h

clean:
	rm -f $(TARGETS) *.o
