all: client server
CC: gcc

server: server.c
	$(CC) -pthread -Wall -Werror -o server server.c

client: client.c
	$(CC) -Wall -Werror -o client client.c

clean:
	@rm -r server
	@rm -r client