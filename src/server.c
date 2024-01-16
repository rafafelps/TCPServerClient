#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

int main(int argc, char* argv[]) {
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

    int clientSocket = accept(serverSocket, NULL, NULL);

    send(clientSocket, serverMessage, sizeof(serverMessage), 0);

    close(serverSocket);

    return 0;
}