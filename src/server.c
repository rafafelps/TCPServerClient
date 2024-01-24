#include "tcpserver.h"

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

    // Set SO_REUSEADDR option to allow the reuse of the address
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

    // Fill all files informations into server data
    server.filesHead = NULL;
    DIR* dir = opendir("files");
    if (dir != NULL) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
                struct File* file = createFileNode(entry->d_name);
                if (file != NULL)
                    insertFileNode(&server.filesHead, file);
            }
        }
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
        scanf("%4s", message);
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

        close(server.clients[i]->socket);
        free(server.clients[i]);
        server.clients[i] == NULL;
        server.connectedClients--;
    }

    close(server.socket);
    return 0;
}
