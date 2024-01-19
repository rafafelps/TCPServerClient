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

// Verifies for valid client console arguments
int isValidClientCommand(int argc, char* argv[]);

// Verifies for valid server console arguments
int isValidServerCommand(int argc, char* argv[]);

// Connects client with the server and handle errors
int connectClient(int clientSocket, struct sockaddr_in* serverAddress);
