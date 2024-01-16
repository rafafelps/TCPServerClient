#include "tcpnet.h"

int main(int argc, char* argv[]) {
    if (!isValidClientCommand(argc, argv)) { return -1; }

    // Creates socket and sockaddr
    int clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == -1) { return -1; }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &serverAddress.sin_addr);

    printf("Attempting to connect to %s:%s...\n", argv[1], argv[2]);
    if (connectClient(clientSocket, &serverAddress) == -1) { return -1;}

    // Recieve data from server
    char serverResponse[256];
    if (recv(clientSocket, &serverResponse, sizeof(serverResponse), 0) == -1) {
        printf("Error receiving server data.\n");
        close(clientSocket);
        return -1;
    }

    printf("Server sent the data: %s\n", serverResponse);

    close(clientSocket);
    return 0;
}