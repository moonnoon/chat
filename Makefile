all: server client
server: server.c
	gcc -g server.c -o server
client: client.c
	gcc -g client.c -o client -lpthread
clean:
	rm server client
