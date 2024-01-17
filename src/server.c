#include <ctype.h>
#include <pthread.h>
#include "tcpnet.h"

#define MAX_CLIENTS 1

struct Client {
    int socket;
    struct sockaddr_in address;
    socklen_t addressLength;
};

struct ServerData {
    int socket;
    struct sockaddr_in address;
    struct Client* clients[MAX_CLIENTS];
    uint8_t connectedClients;
    uint8_t isRunning;
};

void* acceptConnections(struct ServerData* server);

int main(int argc, char* argv[]) {
    if (!isValidServerCommand(argc, argv)) { return 0; }

    struct ServerData server;

    // Fills socket and sockaddr
    server.socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server.socket < 0) { return -1; }

    server.address.sin_family = AF_INET;
    server.address.sin_port = htons(atoi(argv[1]));
    server.address.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to server address
    if (bind(server.socket, (struct sockaddr*)&server.address, sizeof(server.address)) < 0) {
        printf("Error binding the socket.\n");
        close(server.socket);
        return -1;
    }

    // Listen for incoming connections
    if (listen(server.socket, 5) < 0) {
        printf("Error listening to port.\n");
        close(server.socket);
        return -1;
    }

    printf("Server listening on port %s...\n", argv[1]);

    for (uint32_t i = 0; i < MAX_CLIENTS; i++) {
        server.clients[i] = NULL;
    }
    server.connectedClients = 0;

    server.isRunning = 1;
    pthread_t connThread;
    pthread_create(&connThread, NULL, acceptConnections, &server);
    //acceptConnections(&server);
    
    // Exit command
    char message[5] = {'\n'};
    while (strcmp(message, "exit")) {
        scanf("%s", message);
        message[4] = '\n';
    }
    server.isRunning = 0;
    pthread_join(connThread, NULL);

    // Disconnects all clients
    for (uint32_t i = 0; i < MAX_CLIENTS; i++) {
        if (server.clients[i] == NULL) {
            continue;
        }

        // Send message to client
        char message[] = "clsd";
        send(server.clients[i]->socket, message, strlen(message), 0);

        close(server.clients[i]->socket);
        free(server.clients[i]);
        server.clients[i] == NULL;
    }

    close(server.socket);
    return 0;
}

void* acceptConnections(struct ServerData* server) {
    while (server->isRunning) {
        struct Client* client = (struct Client*)malloc(sizeof(struct Client));
        if (client == NULL) {
            printf("Failed to allocate memory for client.\n");
            continue;
        }

        // Accept new client
        client->addressLength = sizeof(client->address);
        client->socket = accept(server->socket, (struct sockaddr*)&client->address, &client->addressLength);
        if (client->socket < 0) {
            printf("Failed to accept connection from %s:%d\n", inet_ntoa(client->address.sin_addr), ntohs(client->address.sin_port));
            continue;
        }

        // Check server capacity
        if (server->connectedClients >= MAX_CLIENTS) {
            // Send server full message
            char message[] = "full";
            send(client->socket, message, strlen(message), 0);

            printf("Failed connection attempt. Server is full.\n");

            close(client->socket);
            continue;
        }

        // Add client to server list
        for (uint32_t i = 0; i < MAX_CLIENTS; i++) {
            if (server->clients[i] == NULL) {
                server->clients[i] = client;
                server->connectedClients++;
                break;
            }
        }
        printf("\nConnection accepted from %s:%d\nTotal connected clients: %d\n\n", inet_ntoa(client->address.sin_addr), ntohs(client->address.sin_port), server->connectedClients);
        
        // Spawn client handler trhread
    }

    return NULL;
}
