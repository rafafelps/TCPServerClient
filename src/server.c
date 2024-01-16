#include "tcpnet.h"

int main(int argc, char* argv[]) {
    if (!isValidServerCommand(argc, argv)) { return 0; }

    // Creates socket and sockaddr
    int serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == -1) { return -1; }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(atoi(argv[1]));
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to server address
    bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

    listen(serverSocket, 5);

    printf("Server listening on port %s...\n", argv[1]);

    int clientSocket = accept(serverSocket, NULL, NULL);
    printf("\n");
    printf("Client %d connected.\n", clientSocket);

    char serverMessage[256] = "You have reached the server!";
    send(clientSocket, serverMessage, sizeof(serverMessage), 0);
    printf("Message sent to client.\n");

    close(clientSocket);
    close(serverSocket);
    return 0;
}