#include "tcpnet.h"

int main(int argc, char* argv[]) {
    if (!isValidCommand(argc, argv)) { return 0; }

    char serverMessage[256] = "You have reached the server!";

    // Creating socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    // Define server address
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(9002);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to server address
    bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

    listen(serverSocket, 5);

    printf("Server started listening on port %s...\n", argv[2]);

    int clientSocket = accept(serverSocket, NULL, NULL);
    printf("\n");
    printf("Client %d connected.\n", clientSocket);

    send(clientSocket, serverMessage, sizeof(serverMessage), 0);
    printf("Message sent to client.\n");

    close(serverSocket);

    return 0;
}