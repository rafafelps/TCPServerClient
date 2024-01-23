#include "tcpserver.h"

int isValidServerCommand(int argc, char* argv[]) {
    // Check arguments length
    if (argc != 2) {
        char* program = strrchr(argv[0], '\\');

        if (program == NULL) {
            program = argv[0];
            printf("Error. Command usage: %s [port]\n", program);
        } else {
            printf("Error. Command usage: .%s [port]\n", program);
        }
        
        return 0;
    }

    // Check for valid port format
    for (int i = 0; argv[1][i] != '\n' && argv[1][i] != '\0'; i++) {
        if (argv[1][i] < '0' || argv[1][i] > '9') {
            printf("Error. Wrong port format.\n");
            return 0;
        }
    }
    int result = atoi(argv[1]);
    if (result < 0 || result > 65535) {
        printf("Error. Wrong port format.\n");
        return 0;
    }
}

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

// File doubly linked list functions

struct File* createFileNode(char* filename) {
    char* filePath = (char*)malloc(strlen(filename) + 7);
    if (filePath == NULL) { return NULL; }

    snprintf(filePath, strlen(filename) + 7, "%s/%s", "files", filename);

    struct File* newFile = (struct File*)malloc(sizeof(struct File));
    if (newFile == NULL) { free(filePath); return NULL; }
    
    // Use stat to get file size
    struct stat fileStat;
    if (!stat(filePath, &fileStat)) {
        newFile->bytes = (long)fileStat.st_size;
    } else {
        newFile->bytes = 0;
    }

    newFile->name = (char*)malloc(strlen(filename) + 1);
    if (newFile->name == NULL) {
        free(filePath);
        free(newFile);
        return NULL;
    }
    strcpy(newFile->name, filename);

    newFile->inUse = 0;
    newFile->next = NULL;
    newFile->prev = NULL;

    free(filePath);
    return newFile;
}

void insertFileNode(struct File** head, struct File* newFile) {
    if (*head == NULL) {
        *head = newFile;
        return;
    }

    struct File* p = *head;
    while (p->next != NULL) {
        p = p->next;
    }

    p->next = newFile;
    newFile->prev = p;
}

struct File* getFileNode(struct File* head, char* filename) {
    if (head == NULL) { return NULL; }

    while(strcmp(head->name, filename)) {
        head = head->next;
        if (head == NULL) { return NULL; }
    }

    return head;
}

void removeFileNode(struct File** head, struct File* file) {
    if (*head == NULL || file == NULL) { return; }

    if (*head == file) {
        *head = file->next;
    }

    if (file->prev != NULL) {
        file->prev->next = file->next;
    }

    if (file->next != NULL) {
        file->next->prev = file->prev;
    }

    free(file->name);
    free(file);
}


