CC=gcc
CFLAGS=-Wall -lpthread -Ilib -Ilib/llist -Isrc
CFLAGSDBG=-g $(CFLAGS)

TARGET=build
TEST=test

all: release

target:
	mkdir -p $(TARGET)/release

client: target
	$(CC) $(CFLAGS) lib/*.c lib/llist/*.c src/client.c -o $(TARGET)/release/client
server: target
	$(CC) $(CFLAGS) lib/*.c lib/llist/*.c src/server.c -o $(TARGET)/release/server

release: client server
	for number in 1 2 3 ; do \
		mkdir -p $(TEST)/$$number ; \
		cp $(TARGET)/release/client $(TEST)/$$number ; \
	done

target_dbg:
	mkdir -p $(TARGET)/debug

client_dbg: target_dbg
	$(CC) $(CFLAGSDBG) lib/*.c lib/llist/*.c src/client.c -o $(TARGET)/debug/client
server_dbg: target_dbg
	$(CC) $(CFLAGSDBG) lib/*.c lib/llist/*.c src/server.c -o $(TARGET)/debug/server

debug: client_dbg server_dbg
	for number in 1 2 3 ; do \
		mkdir -p $(TEST)/$$number ; \
		cp $(TARGET)/debug/client $(TEST)/$$number ; \
	done

clean:
	rm -rf $(TEST) $(TARGET)
