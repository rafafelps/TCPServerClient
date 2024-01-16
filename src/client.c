#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

int main(int argc, char* argv[]) {
    // Creating socket
    int networkSocket = socket(AF_INET, SOCK_STREAM, 0);

    // Specifying address for the socket
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(9002);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    int connStatus = connect(networkSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    if (connStatus == -1)  {
        printf("Error connecting to the remote socket.\n");
        return -1;
    }

    // Recieve data from server
    char serverResponse[256];
    recv(networkSocket, &serverResponse, sizeof(serverResponse), 0);

    printf("Server sent the data: %s\n", serverResponse);

    close(networkSocket);
    return 0;
}