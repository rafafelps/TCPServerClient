all:
	gcc src/tcpnet.h src/tcpnet.c src/client.c -o client
	gcc src/tcpnet.h src/tcpnet.c src/server.c -o server

clean:
	rm -f client
	rm -f server