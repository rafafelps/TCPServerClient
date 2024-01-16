#include "tcpnet.h"

int main(int argc, char* argv[]) {
    if (!isValidCommand(argc, argv)) { return -1; }

    // Creates socket and sockaddr
    struct TCPSock* server = createTCPSocket(argv[1], argv[2]);
    if (server == NULL) { return -1; }

    printf("Attempting to connect to %s:%s...\n", argv[1], argv[2]);
    if (connectClient(server) == -1) { return -1;}

    // Recieve data from server
    char serverResponse[256];
    recv(server->socket, &serverResponse, sizeof(serverResponse), 0);

    printf("Server sent the data: %s\n", serverResponse);

    close(server->socket);
    return 0;
}