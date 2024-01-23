all:
	gcc src/tcpclient.h src/tcpclient.c src/client.c -o client
	gcc src/tcpserver.h src/tcpserver.c src/server.c -o server

clean:
	rm -f client
	rm -f server