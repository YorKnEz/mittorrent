CC=gcc
CFLAGS=-lpthread -Ilib -Ilib/llist -Isrc
CFLAGSDBG=-g $(CFLAGS)

all: release

client:
	$(CC) $(CFLAGS) lib/*.c lib/llist/*.c src/client.c -o build/release/client
client_dbg:
	$(CC) $(CFLAGSDBG) lib/*.c lib/llist/*.c src/client.c -o build/debug/client

server:
	$(CC) $(CFLAGS) lib/*.c lib/llist/*.c src/server.c -o build/release/server
server_dbg:
	$(CC) $(CFLAGSDBG) lib/*.c lib/llist/*.c src/server.c -o build/debug/server

release: client server
	for number in 1 2 3 ; do \
		mkdir -p test/$$number ; \
		cp build/release/client test/$$number ; \
	done

debug: client_dbg server_dbg
	for number in 1 2 3 ; do \
		mkdir -p test/$$number ; \
		cp build/debug/client test/$$number ; \
	done

clean:
	rm -rf test build/release/* build/debug/*