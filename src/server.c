#include "tcpnet.h"

int main(int argc, char* argv[]) {
    if (!isValidCommand(argc, argv)) { return 0; }

    // Creates socket and sockaddr
    struct TCPSock* server = createTCPSocket(argv[1], argv[2]);
    if (server == NULL) { return 0; }

    // Bind the socket to server address
    bind(server->socket, (struct sockaddr*)&server->sockaddr, sizeof(server->sockaddr));

    listen(server->socket, 5);

    printf("Server listening on port %s...\n", argv[2]);

    int clientSocket = accept(server->socket, NULL, NULL);
    printf("\n");
    printf("Client %d connected.\n", clientSocket);

    char serverMessage[256] = "You have reached the server!";
    send(clientSocket, serverMessage, sizeof(serverMessage), 0);
    printf("Message sent to client.\n");

    close(server->socket);
    return 0;
}