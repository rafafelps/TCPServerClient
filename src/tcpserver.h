#ifndef TCP_SERVER
#define TCP_SERVER

#define _GNU_SOURCE
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

#define MAX_CLIENTS 2
#define AUTH "ohygIf2YICKdNafb5YePqgI02EuI6Cd"

struct File {
    char* name;
    long bytes;
    uint8_t inUse;
    struct File* next;
    struct File* prev;
};

struct Client {
    int socket;
    struct sockaddr_in address;
    socklen_t addressLength;
};

struct ServerData {
    int socket;
    struct sockaddr_in address;
    struct Client* clients[MAX_CLIENTS];
    struct Client* currentClient;
    uint8_t connectedClients;
    struct File* filesHead;
    uint8_t isRunning;
};

// Verifies for valid server console arguments
int isValidServerCommand(int argc, char* argv[]);

// Accepts new connections, adds them to the server list
// and creates a new thread for each connection.
// Parameter is struct ServerData*
void* handleConnections(void* sv);

void* handleClient(void* cl);

void sendFilenamesToClient(int clientSocket);

int receiveFileFromClient(struct ServerData* server, int clientSocket);

int sendFileToTheClient(struct ServerData* server, int clientSocket);

int deleteFile(struct ServerData* server, int clientSocket);

// File doubly linked list functions
struct File* createFileNode(char* filename);
void insertFileNode(struct File** head, struct File* newFile);
struct File* getFileNode(struct File* head, char* filename);
void removeFileNode(struct File** head, struct File* file);

#endif
