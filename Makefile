all: server client.so
client.so: client.o
	gcc -shared client.o -o client.so
client.o: client.c chat.h
	gcc -c -fPIC client.c
server: server.c chat.h
	gcc -g server.c -o server
#client: client.c chat.h
#	gcc -g client.c -o client
clean:
	rm -f server client.o client.so
