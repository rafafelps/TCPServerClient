#include <ctype.h>
#include <pthread.h>
#include "tcpnet.h"

#define MAX_CLIENTS 1
#define AUTH "ohygIf2YICKdNafb5YePqgI02EuI6Cd"

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

void* acceptConnections(void* server);

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
    char message[5] = {'\0'};
    while (strcmp(message, "exit")) {
        scanf("%s", message);
        message[4] = '\0';

        for (uint8_t i = 0; message[i] != '\0'; i++) {
            message[i] = tolower(message[i]);
        }
    }
    server.isRunning = 0;

    // Save the current stdout file descriptor
    int old_stdout = dup(1);

    // Redirect stdout to /dev/null or a temporary file
    freopen("/dev/null", "w", stdout);
    fclose(stdout);

    // Creates a fake connection to unblock connect()
    int tmpSock = socket(AF_INET, SOCK_STREAM, 0);
    close(connect(tmpSock, (struct sockaddr*)&server.address, sizeof(server.address)));
    close(tmpSock);
    pthread_join(connThread, NULL);

    // Restore stdout to its original state
    stdout = fdopen(old_stdout, "w");

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

void* acceptConnections(void* server) {
    struct ServerData* sv = (struct ServerData*)server;
    while (sv->isRunning) {
        struct Client* client = (struct Client*)malloc(sizeof(struct Client));
        if (client == NULL) {
            printf("\nFailed to allocate memory for client.\n");
            continue;
        }

        // Accept new client
        client->addressLength = sizeof(client->address);
        client->socket = accept(sv->socket, (struct sockaddr*)&client->address, &client->addressLength);
        if (client->socket < 0) {
            printf("\nFailed to accept connection from %s:%d\n", inet_ntoa(client->address.sin_addr), ntohs(client->address.sin_port));
            continue;
        }

        // Check server capacity
        if (sv->connectedClients >= MAX_CLIENTS) {
            // Send server full message
            send(client->socket, "full", strlen("full"), 0);

            printf("\nRefused connection. Server is full.\n");

            close(client->socket);
            continue;
        }

        // Validate client
        char auth[32] = {'\0'};
        recv(client->socket, auth, 32, 0);
        auth[31] = '\0';
        if (strcmp(auth, AUTH)) {
            // Send authentication error message
            send(client->socket, "auth", strlen("auth"), 0);

            printf("\nRefused connection. Wrong authentication code.\n");

            close(client->socket);
            continue;
        }

        // Add client to server list
        for (uint32_t i = 0; i < MAX_CLIENTS; i++) {
            if (sv->clients[i] == NULL) {
                sv->clients[i] = client;
                sv->connectedClients++;
                break;
            }
        }
        printf("\nConnection accepted from %s:%d\nTotal connected clients: %d\n\n", inet_ntoa(client->address.sin_addr), ntohs(client->address.sin_port), sv->connectedClients);
        
        // Spawn client handler trhread
    }

    return NULL;
}
