all: server.o client.o
	clang server.o -o server
	clang client.o -o client

server.o: server.c common.h
	clang -c server.c

client.o: client.c common.h
	clang -c client.c

clean:
	rm -f server.o client.o server client
