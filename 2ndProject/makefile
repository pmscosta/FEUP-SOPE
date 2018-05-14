all: client server

server: server.c
	@gcc -pthread -Wall -Werror -o server server.c

client: client.c
	@gcc -Wall -Werror -o client client.c

clean:
	@rm -r server
	@rm -r client