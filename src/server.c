#include "tcpserver.h"

#define MAX_CLIENTS 2
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
    struct File* filesHead;
    uint8_t isRunning;
};

void* handleConnections(void* sv);
void* handleClient(void* cl);
void sendFilenamesToClient(int clientSocket);
int receiveFileFromClient(struct ServerData* server, int clientSocket);
int sendFileToTheClient(struct ServerData* server, int clientSocket);
int deleteFile(struct ServerData* server, int clientSocket);

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
        recv(client->socket, auth, 31, 0);
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
            if (receiveFileFromClient(server, client->socket)) {
                printf("%s:%d uploaded a file to the server.\n", inet_ntoa(client->address.sin_addr), ntohs(client->address.sin_port));
            }
        } else if (!strcmp(message, "down")) {
            // Download function
            if (sendFileToTheClient(server, client->socket)) {
                printf("%s:%d downloaded a file from the server.\n", inet_ntoa(client->address.sin_addr), ntohs(client->address.sin_port));
            }
        } else if (!strcmp(message, "dlfl")) {
            // Delete file function
            if (deleteFile(server, client->socket)) {
                printf("%s:%d deleted a file from the server.\n", inet_ntoa(client->address.sin_addr), ntohs(client->address.sin_port));
            }
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

    struct dirent** namelist;
    int n = scandir("files", &namelist, 0, versionsort);
    if (n < 0) {
        printf("Error opening directory.\n");
        return;
    }
    for (int i = 0; i < n; i++) {
        if (strcmp(namelist[i]->d_name, ".") && strcmp(namelist[i]->d_name, "..")) {
            ssize_t msgLen = strlen(namelist[i]->d_name);
            send(clientSocket, &msgLen, sizeof(ssize_t), 0);
            send(clientSocket, namelist[i]->d_name, msgLen, 0);
        }
        free(namelist[i]);
    }
    free(namelist);

    ssize_t msgLen = 3;
    send(clientSocket, &msgLen, sizeof(ssize_t), 0);
    send(clientSocket, "end", msgLen, 0);
    return;
}

int deleteFile(struct ServerData* server, int clientSocket) {
    // Get filename
    ssize_t fileLen = 0;
    if (recv(clientSocket, &fileLen, sizeof(ssize_t), 0) <= 0) {
        fileLen = 5;
        send(clientSocket, &fileLen, sizeof(ssize_t), 0);
        send(clientSocket, "error", fileLen, 0);
        return 0;
    }
    if (fileLen <= 0) {
        fileLen = 5;
        send(clientSocket, &fileLen, sizeof(ssize_t), 0);
        send(clientSocket, "error", fileLen, 0);
        return 0;
    }

    char* filename = (char*)malloc(fileLen + 1);
    if (filename == NULL) {
        fileLen = 5;
        send(clientSocket, &fileLen, sizeof(ssize_t), 0);
        send(clientSocket, "error", fileLen, 0);
        return 0;
    }

    if (recv(clientSocket, filename, fileLen, 0) <= 0) {
        fileLen = 5;
        send(clientSocket, &fileLen, sizeof(ssize_t), 0);
        send(clientSocket, "error", fileLen, 0);
        free(filename);
        return 0;
    }
    filename[fileLen] = '\0';

    // Check file existance
    struct File* file = getFileNode(server->filesHead, filename);
    if (file == NULL) {
        fileLen = 4;
        send(clientSocket, &fileLen, sizeof(ssize_t), 0);
        send(clientSocket, "nfnd", fileLen, 0);
        free(filename);
        return 0;
    }

    // Delete file
    char* filePath = (char*)malloc(fileLen + 7);
    if (filePath == NULL) {
        fileLen = 5;
        send(clientSocket, &fileLen, sizeof(ssize_t), 0);
        send(clientSocket, "error", fileLen, 0);
        free(filename);
        return 0;
    }

    if (file->inUse) {
        fileLen = 5;
        send(clientSocket, &fileLen, sizeof(ssize_t), 0);
        send(clientSocket, "error", fileLen, 0);

        free(filename);
        free(filePath);
        return 0;
    }

    snprintf(filePath, fileLen + 7, "%s/%s", "files", filename);
    if (remove(filePath) != 0) {
        fileLen = 5;
        send(clientSocket, &fileLen, sizeof(ssize_t), 0);
        send(clientSocket, "error", fileLen, 0);

        free(filename);
        free(filePath);
        return 0;
    }

    fileLen = 3;
    send(clientSocket, &fileLen, sizeof(ssize_t), 0);
    send(clientSocket, "end", fileLen, 0);

    // Remove file from server list
    removeFileNode(&server->filesHead, file);

    free(filename);
    free(filePath);
    return 1;
}

int receiveFileFromClient(struct ServerData* server, int clientSocket) {
    // Get filename from client
    ssize_t filenameLen = 0;
    if (recv(clientSocket, &filenameLen, sizeof(ssize_t), 0) <= 0) {
        filenameLen = 5;
        send(clientSocket, &filenameLen, sizeof(ssize_t), 0);
        send(clientSocket, "error", filenameLen, 0);
        return 0;
    }
    
    char* filename = (char*)malloc(filenameLen + 1);
    filename[filenameLen] = '\0';
    if (filename == NULL) {
        filenameLen = 5;
        send(clientSocket, &filenameLen, sizeof(ssize_t), 0);
        send(clientSocket, "error", filenameLen, 0);
        return 0;
    }

    if (recv(clientSocket, filename, filenameLen, 0) <= 0) {
        filenameLen = 5;
        send(clientSocket, &filenameLen, sizeof(ssize_t), 0);
        send(clientSocket, "error", filenameLen, 0);
        return 0;
    }

    // Check if file already exists
    struct File* file = getFileNode(server->filesHead, filename);
    if (file != NULL) {
        filenameLen = 5;
        send(clientSocket, &filenameLen, sizeof(ssize_t), 0);
        send(clientSocket, "exist", filenameLen, 0);
        free(filename);
        return 0;
    }
    filenameLen = 9;
    send(clientSocket, &filenameLen, sizeof(ssize_t), 0);
    send(clientSocket, "confirmed", filenameLen, 0);

    // Create new file and append it to the server list
    file = createFileNode(filename);
    insertFileNode(&server->filesHead, file);
    file->inUse = 1;

    // Receive file size
    ssize_t fileSize = 0;
    if (recv(clientSocket, &fileSize, sizeof(ssize_t), 0) <= 0) {
        filenameLen = 5;
        send(clientSocket, &filenameLen, sizeof(ssize_t), 0);
        send(clientSocket, "error", filenameLen, 0);
        free(filename);
        removeFileNode(&server->filesHead, file);
        return 0;
    }
    file->bytes = fileSize;

    // Create "files" folder if it doesn't exists
    struct stat st;
    if (stat("files", &st)) {
        if (mkdir("files", 0777)) {
            printf("Failed to create \"files/\" directory.\n");
            filenameLen = 5;
            send(clientSocket, &filenameLen, sizeof(ssize_t), 0);
            send(clientSocket, "error", filenameLen, 0);
            free(filename);
            removeFileNode(&server->filesHead, file);
            return 0;
        }
    }

    // Open the file for writing
    char* filePath = (char*)malloc(strlen(filename) + 7);
    if (filePath == NULL) {
        filenameLen = 5;
        send(clientSocket, &filenameLen, sizeof(ssize_t), 0);
        send(clientSocket, "error", filenameLen, 0);
        free(filename);
        removeFileNode(&server->filesHead, file);
        return 0;
    }

    snprintf(filePath, strlen(filename) + 7, "%s/%s", "files", filename);
    int fileDescriptor = open(filePath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fileDescriptor < 0) {
        filenameLen = 5;
        send(clientSocket, &filenameLen, sizeof(ssize_t), 0);
        send(clientSocket, "error", filenameLen, 0);
        free(filename);
        free(filePath);
        removeFileNode(&server->filesHead, file);
        return 0;
    }

    // Receive and write the file content
    char buffer[1024];
    ssize_t bytesRead;
    ssize_t totalBytesRead = 0;

    while (totalBytesRead != fileSize) {
        if ((bytesRead = recv(clientSocket, buffer, 1024, 0)) <= 0) {
            filenameLen = 5;
            send(clientSocket, &filenameLen, sizeof(ssize_t), 0);
            send(clientSocket, "error", filenameLen, 0);
            removeFileNode(&server->filesHead, file);
            close(fileDescriptor);
            remove(filePath);
            free(filename);
            free(filePath);
            return 0;
        }
        if (write(fileDescriptor, buffer, bytesRead) <= 0) {
            filenameLen = 5;
            send(clientSocket, &filenameLen, sizeof(ssize_t), 0);
            send(clientSocket, "error", filenameLen, 0);
            removeFileNode(&server->filesHead, file);
            close(fileDescriptor);
            remove(filePath);
            free(filename);
            free(filePath);
            return 0;
        }
        totalBytesRead += bytesRead;
    }

    // Send confirmation to the client
    if (totalBytesRead == fileSize) {
        filenameLen = 7;
        send(clientSocket, &filenameLen, sizeof(ssize_t), 0);
        send(clientSocket, "success", filenameLen, 0);
    } else {
        filenameLen = 5;
        send(clientSocket, &filenameLen, sizeof(ssize_t), 0);
        send(clientSocket, "error", filenameLen, 0);
        removeFileNode(&server->filesHead, file);
        close(fileDescriptor);
        remove(filePath);
        free(filename);
        free(filePath);
        return 0;
    }

    file->inUse = 0;
    close(fileDescriptor);
    free(filename);
    free(filePath);
    return 1;
}

int sendFileToTheClient(struct ServerData* server, int clientSocket) {
    // Get filename from the client
    ssize_t filenameLen = 0;
    if (recv(clientSocket, &filenameLen, sizeof(ssize_t), 0) <= 0) {
        filenameLen = 5;
        send(clientSocket, &filenameLen, sizeof(ssize_t), 0);
        send(clientSocket, "error", filenameLen, 0);
        return 0;
    }

    char* filename = (char*)malloc(filenameLen + 1);
    if (filename == NULL) {
        filenameLen = 5;
        send(clientSocket, &filenameLen, sizeof(ssize_t), 0);
        send(clientSocket, "error", filenameLen, 0);
        return 0;
    }
    filename[filenameLen] = '\0';

    if (recv(clientSocket, filename, filenameLen, 0) <= 0) {
        filenameLen = 5;
        send(clientSocket, &filenameLen, sizeof(ssize_t), 0);
        send(clientSocket, "error", filenameLen, 0);
        free(filename);
        return 0;
    }

    // Get information about file
    struct File* file = getFileNode(server->filesHead, filename);
    if (file == NULL) {
        filenameLen = 5;
        send(clientSocket, &filenameLen, sizeof(ssize_t), 0);
        send(clientSocket, "exist", filenameLen, 0);
        free(filename);
        return 0;
    }
    free(filename);
    file->inUse = 1;

    // Send file existance confirmation
    filenameLen = 9;
    send(clientSocket, &filenameLen, sizeof(ssize_t), 0);
    send(clientSocket, "confirmed", filenameLen, 0);

    // Send file size to the client
    send(clientSocket, &file->bytes, sizeof(ssize_t), 0);

    char* filePath = (char*)malloc(strlen(filename) + 7);
    if (filePath == NULL) {
        filenameLen = 5;
        send(clientSocket, &filenameLen, sizeof(ssize_t), 0);
        send(clientSocket, "error", filenameLen, 0);
        return 0;
    }

    snprintf(filePath, strlen(filename) + 7, "%s/%s", "files", file->name);

    // Open the file for reading
    int fileDescriptor = open(filePath, O_RDONLY);
    if (fileDescriptor < 0) {
        filenameLen = 5;
        send(clientSocket, &filenameLen, sizeof(ssize_t), 0);
        send(clientSocket, "error", filenameLen, 0);
        return 0;
    }

    // Read and send the file content
    char buffer[1024];
    ssize_t bytesRead;
    ssize_t totalBytesRead = 0;

    while ((bytesRead = read(fileDescriptor, buffer, 1024)) > 0) {
        if (send(clientSocket, buffer, bytesRead, 0) <= 0) {
            close(fileDescriptor);
            free(filePath);
            return 0;
        }
        totalBytesRead += bytesRead;
    }

    if (totalBytesRead != file->bytes) {
        filenameLen = 5;
        send(clientSocket, &filenameLen, sizeof(ssize_t), 0);
        send(clientSocket, "error", filenameLen, 0);
        return 0;
    }

    file->inUse = 0;
    close(fileDescriptor);
    free(filePath);
    return 1;
}
