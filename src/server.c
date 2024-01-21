#include "tcpnet.h"

#define MAX_CLIENTS 2
#define MAX_BUFFER_SIZE 64
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
    struct Client* currentClient;
    uint8_t connectedClients;
    uint8_t isRunning;
};

void* handleConnections(void* sv);
void* handleClient(void* cl);
void sendFilenamesToClient(int clientSocket);

int main(int argc, char* argv[]) {
    if (!isValidServerCommand(argc, argv)) { return 0; }

    struct ServerData server;

    // Fills socket and sockaddr
    server.socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server.socket < 0) {
        printf("Error creating server socket.\n");
        return -1;
    }

    server.address.sin_family = AF_INET;
    server.address.sin_port = htons(atoi(argv[1]));
    server.address.sin_addr.s_addr = INADDR_ANY;

    // Set SO_REUSEADDR option to allow reuse of the address
    int reuseaddr = 1;
    if (setsockopt(server.socket, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int)) < 0) {
        printf("Error setsockopt failed.\n");
        close(server.socket);
        return -1;
    }

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

    // Spawn connection handler thread
    server.isRunning = 1;
    pthread_t connThread;
    pthread_create(&connThread, NULL, handleConnections, &server);
    
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

    // Redirect stdout to /dev/null
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
        if (server.clients[i] == NULL) { continue; }

        // Send server closed message to client
        send(server.clients[i]->socket, "clsd", strlen("clsd"), 0);

        close(server.clients[i]->socket);
        free(server.clients[i]);
        server.clients[i] == NULL;
        server.connectedClients--;
    }

    close(server.socket);
    return 0;
}

// Accepts new connections, adds them to the server list
// and creates a new thread for each connection.
// Parameter is struct ServerData*
void* handleConnections(void* sv) {
    struct ServerData* server = (struct ServerData*)sv;
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
            free(client);
            continue;
        }

        // Check server capacity
        if (server->connectedClients >= MAX_CLIENTS) {
            // Send server full message
            send(client->socket, "full", strlen("full"), 0);

            printf("Refused connection. Server is full.\n");

            close(client->socket);
            free(client);
            continue;
        }

        // Validate client
        char auth[32] = {'\0'};
        recv(client->socket, auth, 32, 0);
        auth[31] = '\0';
        if (strcmp(auth, AUTH)) {
            // Send authentication error message
            send(client->socket, "auth", strlen("auth"), 0);

            printf("Refused connection from %s:%d. Wrong authentication code.\n", inet_ntoa(client->address.sin_addr), ntohs(client->address.sin_port));

            close(client->socket);
            free(client);
            continue;
        }

        // Add client to server list
        for (uint32_t i = 0; i < MAX_CLIENTS; i++) {
            if (server->clients[i] == NULL) {
                server->clients[i] = client;
                server->connectedClients++;
                server->currentClient = client;
                break;
            }
        }

        printf("\nConnection accepted from %s:%d\n", inet_ntoa(client->address.sin_addr), ntohs(client->address.sin_port));
        printf("Total connected clients: %d\n\n", server->connectedClients);

        // Send connection successful status to client
        send(client->socket, "conn", strlen("conn"), 0);

        // Spawn client handler thread
        pthread_t id;
        pthread_create(&id, NULL, handleClient, server);
    }

    return NULL;
}

void* handleClient(void* sv) {
    struct ServerData* server = (struct ServerData*)sv;
    struct Client* client = server->currentClient;

    char message[5] = {'\0'};
    while (server->isRunning) {
        ssize_t status = recv(client->socket, message, 4, 0);

        // Check disconnected client
        if (!status) {
            printf("\nClient %s:%d disconnected from the server.\n", inet_ntoa(client->address.sin_addr), ntohs(client->address.sin_port));

            // Remove client from server list
            for (uint32_t i = 0; i < MAX_CLIENTS; i++) {
                if (server->clients[i] == NULL) { continue; }

                if (server->clients[i]->socket == client->socket) {
                    close(server->clients[i]->socket);
                    free(server->clients[i]);
                    server->clients[i] = NULL;
                    server->connectedClients--;
                    break;
                }
            }

            printf("Total connected clients: %d\n\n", server->connectedClients);

            break;
        }

        // Check client message and call a specific action
        if (!strcmp(message, "list")) {
            // List function
            sendFilenamesToClient(client->socket);
            printf("%s:%d listed server files.\n", inet_ntoa(client->address.sin_addr), ntohs(client->address.sin_port));
        } else if (!strcmp(message, "upld")) {
            // Upload function
            printf("%s:%d uploaded a file to the server.\n", inet_ntoa(client->address.sin_addr), ntohs(client->address.sin_port));
        } else if (!strcmp(message, "down")) {
            // Download function
            printf("%s:%d downloaded a file from the server.\n", inet_ntoa(client->address.sin_addr), ntohs(client->address.sin_port));
        } else if (!strcmp(message, "dlfl")) {
            // Delete file function
            printf("%s:%d deleted a file from the server.\n", inet_ntoa(client->address.sin_addr), ntohs(client->address.sin_port));
        }
    }

    return NULL;
}

void sendFilenamesToClient(int clientSocket) {
    // Create "files" folder if it doesn't exists
    struct stat st;
    if (stat("files", &st)) {
        if (mkdir("files", 0777)) {
            printf("Failed to create \"files/\" directory.\n");
            return;
        }
    }

    DIR* dir = opendir("files");
    if (dir == NULL) {
        printf("Error opening directory.\n");
        return;
    }
    
    // Send all filenames to client
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
            ssize_t msgLen = strlen(entry->d_name);
            send(clientSocket, &msgLen, sizeof(ssize_t), 0);
            send(clientSocket, entry->d_name, msgLen, 0);
        }
    }
    
    ssize_t msgLen = 3;
    send(clientSocket, &msgLen, sizeof(ssize_t), 0);
    send(clientSocket, "end", msgLen, 0);
    closedir(dir);
}
