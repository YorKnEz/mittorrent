all:
	gcc -lpthread lib/*.c -Ilib src/client.c -Isrc -o build/release/client
	cp build/release/client test/1
	cp build/release/client test/2
	cp build/release/client test/3
	gcc -lpthread lib/*.c -Ilib src/server.c -Isrc -o build/release/server
debug:
	gcc -lpthread -g lib/*.c -Ilib src/client.c -Isrc -o build/debug/client
	cp build/debug/client test/1
	cp build/debug/client test/2
	cp build/debug/client test/3
	gcc -lpthread -g lib/*.c -Ilib src/server.c -Isrc -o build/debug/server
clean:
	rm build/release/*
	rm build/debug/*