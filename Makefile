all:
	gcc server.c -o server
	gcc clientTCP.c -o client
	clear

clean:
	rm server
	rm client
