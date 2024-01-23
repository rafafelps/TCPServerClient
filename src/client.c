#include "tcpclient.h"

int main(int argc, char* argv[]) {
    if (!isValidClientCommand(argc, argv)) { return -1; }

    struct Client client;

    // Fills socket and sockaddr
    client.socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client.socket < 0) {
        printf("Error creating client socket.\n");
        return -1;
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &serverAddress.sin_addr);

    // Connect to the server
    printf("Attempting to connect to %s:%s...\n", argv[1], argv[2]);
    if (connectClient(client.socket, &serverAddress)) { return -1;}

    // Send authentication code to the server
    send(client.socket, AUTH, strlen(AUTH), 0);
    char svMessage[5] = {'\0'};

    // Get server response
    recv(client.socket, svMessage, 4, 0);
    if (!strcmp(svMessage, "conn")) {
        printf("Connected to the server!\n");
        clientConsole(&client);
    } else if (!strcmp(svMessage, "full")) {
        printf("Server is full.\n");
    } else if (!strcmp(svMessage, "auth")) {
        printf("Wrong authentication code.\n");
    }

    close(client.socket);
    return 0;
}
