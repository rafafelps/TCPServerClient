#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <arpa/inet.h>

struct TCPSock {
    int socket;
    struct sockaddr_in sockaddr;
};

// Verifies for valid console arguments
int isValidCommand(int argc, char* argv[]);

// Creates a TCPSocket structure
struct TCPSock* createTCPSocket(char* ip, char* port);

// Connects client with the server and handle errors
int connectClient(struct TCPSock* server);
