all: server.o client.o
	gcc -o server server.o
	gcc -o client client.o

server.o: server.c chat.h
	gcc -c server.c

client.o: client.c chat.h
	gcc -c client.c

clean:
	rm -f server.o client.o server client
