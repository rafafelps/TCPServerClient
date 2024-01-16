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
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        printf("Error binding the socket.\n");
        close(serverSocket);
        return -1;
    }

    // Listen for incoming connections
    if (listen(serverSocket, 5) == -1) {
        printf("Error listening to port.\n");
        close(serverSocket);
        return -1;
    }

    printf("Server listening on port %s...\n", argv[1]);

    // Accept client connection
    int clientSocket = accept(serverSocket, NULL, NULL);
    printf("\n");
    if (clientSocket == -1) {
        printf("Failed to accept client connection.\n");
        close(clientSocket);
        close(serverSocket);
        return -1;
    }
    printf("Client %d connected.\n", clientSocket);

    // Send server message
    char serverMessage[256] = "You have reached the server!";
    if (send(clientSocket, serverMessage, sizeof(serverMessage), 0) == -1) {
        printf("Failed to sent a message to the client.\n");
    } else {
        printf("Message sent to client.\n");
    }

    close(clientSocket);
    close(serverSocket);
    return 0;
}