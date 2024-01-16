#include "tcpnet.h"

int main(int argc, char* argv[]) {
    if (!isValidCommand(argc, argv)) { return -1; }

    // Creates socket and sockaddr
    struct TCPSock* client = createTCPSocket(argv[1], argv[2]);
    if (client == NULL) { return -1; }

    printf("Attempting to connect to %s:%s...\n", argv[1], argv[2]);
    if (connectClient(client) == -1) { return -1;}

    // Recieve data from server
    char serverResponse[256];
    recv(client->socket, &serverResponse, sizeof(serverResponse), 0);

    printf("Server sent the data: %s\n", serverResponse);

    close(client->socket);
    return 0;
}