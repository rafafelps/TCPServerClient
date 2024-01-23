#ifndef TCP_CLIENT
#define TCP_CLIENT
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>
#include <pthread.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#define AUTH "ohygIf2YICKdNafb5YePqgI02EuI6Cd"
#define COMM_LEN 256

struct Client {
    int socket;
    uint8_t nonBlocking;
};

// Verifies for valid client console arguments
int isValidClientCommand(int argc, char* argv[]);

// Connects client with the server and handle errors
int connectClient(int clientSocket, struct sockaddr_in* serverAddress);

void clientConsole(struct Client* client);

void* detectClosedServer(void* cl);

void getFilenamesFromServer(int socket);

void sendFileToTheServer(int socket, char* path);

void receiveFileFromTheServer(int socket, char* filename);

void deleteFile(int socket, char* filename);

#endif
