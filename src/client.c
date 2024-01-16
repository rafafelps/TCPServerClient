#include "tcpnet.h"

int main(int argc, char* argv[]) {
    if (!isValidCommand(argc, argv)) { return 0; }

    // Creating socket
    int networkSocket = socket(AF_INET, SOCK_STREAM, 0);

    // Specifying address for the socket
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(9002);
    inet_pton(AF_INET, "127.0.0.1", &(serverAddress.sin_addr));

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