#include "tcpnet.h"

#define AUTH "ohygIf2YICKdNafb5YePqgI02EuI6Cd"
#define COMM_LEN 256

struct Client {
    int socket;
    uint8_t nonBlocking;
};

void clientConsole(struct Client* client);
void* detectClosedServer(void* cl);
void getFilenamesFromServer(int socket);
void sendFileToTheServer(int socket, char* path);
void deleteFile(int socket, char* filename);

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

void clientConsole(struct Client* client) {
    char command[COMM_LEN] = {'\0'};
    while (1) {
        // Spawn server closed thread
        client->nonBlocking = 1;
        pthread_t clsdServerThread;
        pthread_create(&clsdServerThread, NULL, detectClosedServer, client);

        printf("\n> ");
        fgets(command, COMM_LEN - 1, stdin);

        // Lowercase first word
        char action[COMM_LEN] = {'\0'};
        uint32_t wordEnd;
        for (wordEnd = 0; command[wordEnd] != '\0'; wordEnd++) {
            if (command[wordEnd] == ' ' || command[wordEnd] == '\n') { break; }

            action[wordEnd] = tolower(command[wordEnd]);
        }

        // Restore blocking mode
        client->nonBlocking = 0;
        pthread_join(clsdServerThread, NULL);

        if (!strcmp(action, "list") || !strcmp(action, "ls")) {
            // List command
            send(client->socket, "list", strlen("list"), 0);
            getFilenamesFromServer(client->socket);
        } else if (!strcmp(action, "upload") || !strcmp(action, "up")) {
            // Get file path from command
            char filePath[COMM_LEN] = {'\0'};
            if (command[wordEnd] == '\0' || command[wordEnd] == '\n') {
                printf("Error. Usage: upload [path]\n");
                continue;
            } else {
                for (uint32_t i = 1; command[wordEnd + i] != '\0'; i++) {
                    if (command[wordEnd + i] == ' ' || command[wordEnd + i] == '\n') { break; }
                    filePath[i - 1] = command[wordEnd + i];
                }
                if (filePath[0] == '\0') {
                    printf("Error. Usage: upload [path]\n");
                    continue;
                }
            }



            // Upload command
            sendFileToTheServer(client->socket, filePath);
        } else if (!strcmp(action, "download") || !strcmp(action, "dwn")) {
            // Get filename from command
            char filename[COMM_LEN] = {'\0'};
            if (command[wordEnd] == '\0' || command[wordEnd] == '\n') {
                printf("Error. Usage: download [filename]\n");
                continue;
            } else {
                for (uint32_t i = 1; command[wordEnd + i] != '\0'; i++) {
                    if (command[wordEnd + i] == ' ' || command[wordEnd + i] == '\n') { break; }
                    filename[i - 1] = command[wordEnd + i];
                }
                if (filename[0] == '\0') {
                    printf("Error. Usage: download [filename]\n");
                    continue;
                }
            }

            // Download command
            send(client->socket, "down", strlen("down"), 0);
        } else if (!strcmp(action, "delete") || !strcmp(action, "del")) {
            // Get filename from command
            char filename[COMM_LEN] = {'\0'};
            if (command[wordEnd] == '\0' || command[wordEnd] == '\n') {
                printf("Error. Usage: delete [filename]\n");
                continue;
            } else {
                for (uint32_t i = 1; command[wordEnd + i] != '\0'; i++) {
                    if (command[wordEnd + i] == ' ' || command[wordEnd + i] == '\n') { break; }
                    filename[i - 1] = command[wordEnd + i];
                }
                if (filename[0] == '\0') {
                    printf("Error. Usage: delete [filename]\n");
                    continue;
                }
            }

            // Delete command
            send(client->socket, "dlfl", strlen("dlfl"), 0);
            deleteFile(client->socket, filename);
        } else if (!strcmp(action, "help") || !strcmp(action, "h")) {
            // Help command
            printf("\n\x1b[1;36m================================ Command List ===============================\x1b[0m\n");
            printf("\x1b[1;34mhelp     (h)   : \x1b[0mShows this information.\n");
            printf("\x1b[1;34mlist     (ls)  : \x1b[0mLists all server files.\n");
            printf("\x1b[1;34mupload   (up)  : \x1b[0mUploads a file to the server. Usage: upload [path]\n");
            printf("\x1b[1;34mdownload (dwn) : \x1b[0mDownloads a file from the server. Usage: download [filename]\n");
            printf("\x1b[1;34mdelete   (del) : \x1b[0mDeletes a file from the server. Usage: delete [filename]\n");
            printf("\x1b[1;34mexit           : \x1b[0mDisconnects from the server and closes the program.\n");
            printf("\x1b[1;36m=============================================================================\x1b[0m\n\n");
        } else if (!strcmp(action, "exit")) {
            return;
        } else {
            printf("Command not found. Type \"help\" or \"h\" for help.\n");
        }
    }
}

void* detectClosedServer(void* cl) {
    struct Client* client = (struct Client*)cl;

    // Set client socket to non blocking mode
    int flags = fcntl(client->socket, F_GETFL, 0);
    if (flags < 0) {
        printf("Error: fcntl F_GETFL.\n");
        close(client->socket);
        exit(-1);
    }

    if (fcntl(client->socket, F_SETFL, flags | O_NONBLOCK) < 0) {
        printf("Error: fcntl F_SETFL O_NONBLOCK.\n");
        close(client->socket);
        exit(-1);
    }

    // Check server for a closed server
    char svMessage[2] = {'\0'};
    while (client->nonBlocking && recv(client->socket, svMessage, 1, 0) != 0) {
        // Limit loop to 4 times a second | 250,000 microseconds
        usleep(250000);
    }

    // Restore back blocking mode
    if (!client->nonBlocking) {
        int flags = fcntl(client->socket, F_GETFL, 0);
        if (flags < 0) {
            printf("Error: fcntl F_GETFL.\n");
            close(client->socket);
            exit(-1);
        }

        if (fcntl(client->socket, F_SETFL, flags & ~O_NONBLOCK) < 0) {
            printf("Error: fcntl F_SETFL ~O_NONBLOCK\n");
            close(client->socket);
            exit(-1);
        }

        return NULL;
    }

    printf("\nServer closed.\n");
    close(client->socket);
    exit(0);
    return NULL;
}

void getFilenamesFromServer(int socket) {
    while (1) {
        ssize_t msgLen = 0;
        if (recv(socket, &msgLen, sizeof(ssize_t), 0) <= 0) {
            printf("\nServer closed.\n");
            close(socket);
            exit(0);
        }

        char* data = (char*)malloc(msgLen + 1);
        if (data == NULL) {
            printf("Failed to allocate memory for filename.\n");
            close(socket);
            exit(-1);
        }

        if (recv(socket, data, msgLen, 0) <= 0) {
            printf("\nServer closed.\n");
            close(socket);
            free(data);
            exit(0);
        }
        data[msgLen] = '\0';

        if (!strcmp(data, "end")) { free(data); break; }

        printf("%s\n", data);
        free(data);
    }
}

void sendFileToTheServer(int socket, char* path) {
    // Get information about file size
    struct stat fileStat;
    if (stat(path, &fileStat)) {
        printf("Couldn\'t find \"%s\"\n", path);
        return;
    }
    ssize_t fileSize = (long)fileStat.st_size;
    if (fileSize < 0) {
        printf("Error getting file size.\n");
        return;
    }

    // Open the file for reading
    int fileDescriptor = open(path, O_RDONLY);
    if (fileDescriptor < 0) {
        printf("Error opening %s\n", path);
        return;
    }

    // Get filename
    size_t pathLen = strlen(path);
    ssize_t filenameLen;
    for (filenameLen = 1; filenameLen <= pathLen; filenameLen++) {
        if (path[pathLen - filenameLen] == '\\' || path[pathLen - filenameLen] == '/') { break; }
    }
    filenameLen--;
    if (filenameLen <= 0 || filenameLen > pathLen) {
        printf("Error getting file name.\n");
        close(fileDescriptor);
        return;
    }

    send(socket, "upld", strlen("upld"), 0);

    // Send filename to the server
    char* filename = path + (pathLen - filenameLen);
    send(socket, &filenameLen, sizeof(ssize_t), 0);
    send(socket, filename, filenameLen, 0);

    // Receive server confirmation
    ssize_t svMessageLen = 0;
    if (recv(socket, &svMessageLen, sizeof(ssize_t), 0) <= 0) {
        printf("\nServer closed.\n");
        close(fileDescriptor);
        close(socket);
        exit(0);
    }
    char* svMessage = (char*)malloc(svMessageLen + 1);
    svMessage[svMessageLen] = '\0';
    if (svMessage == NULL) {
        printf("Error allocating memory for server response.\n");
        close(fileDescriptor);
        close(socket);
        exit(-1);
    }

    if (recv(socket, svMessage, svMessageLen, 0) <= 0) {
        printf("\nServer closed.\n");
        close(fileDescriptor);
        close(socket);
        free(svMessage);
        exit(0);
    }

    if (!strcmp(svMessage, "exist")) {
        printf("This file name already exists on the server.\n");
        close(fileDescriptor);
        free(svMessage);
        return;
    } else if (!strcmp(svMessage, "confirmed")) {
        send(socket, &fileSize, sizeof(ssize_t), 0);
    } else {
        printf("Error getting server confirmation.\n");
        close(fileDescriptor);
        free(svMessage);
        return;
    }
    free(svMessage);

    // Read and send the file content
    char buffer[1024];
    ssize_t bytesRead;
    ssize_t totalBytesRead = 0;

    while ((bytesRead = read(fileDescriptor, buffer, 1024)) > 0) {
        if (send(socket, buffer, bytesRead, 0) < 0) {
            printf("\nServer closed.\n");
            close(fileDescriptor);
            close(socket);
            break;
        }
        totalBytesRead += bytesRead;
    }

    close(fileDescriptor);
    if (totalBytesRead != fileSize) {
        printf("Error. File size sent doesn\'t match with local file.\n");
        close(socket);
        exit(-1);
    }

    // Get server confirmation
    if (recv(socket, &svMessageLen, sizeof(ssize_t), 0) <= 0) {
        printf("\nServer closed.\n");
        close(socket);
        exit(0);
    }

    svMessage = (char*)malloc(svMessageLen + 1);
    svMessage[svMessageLen] = '\0';
    if (svMessage == NULL) {
        printf("Error allocating memory for server response.\n");
        close(socket);
        exit(-1);
    }
    if (recv(socket, svMessage, svMessageLen, 0) <= 0) {
        printf("\nServer closed.\n");
        close(socket);
        free(svMessage);
        exit(0);
    }

    if (!strcmp(svMessage, "success")) {
        printf("File sent to the server successfully!\n");
    } else {
        printf("Something went wrong.\n");
        close(socket);
        free(svMessage);
        exit(-1);
    }

    free(svMessage);
}

int receiveFileFromTheServer(int socket, char* filename) {
    // Send filename to the server
    ssize_t filenameLen = strlen(filename);
    send(socket, &filenameLen, sizeof(ssize_t), 0);
    send(socket, filename, filenameLen, 0);

    // Check if file already exists
    filenameLen = 0;
    if (recv(socket, &filenameLen, sizeof(ssize_t), 0) <= 0) {
        printf("\nServer closed.\n");
        close(socket);
        exit(0);
    }
    char* svMessage = (char*)malloc(filenameLen + 1);
    if (svMessage == NULL) {
        printf("Error allocating memory for server response.\n");
        close(socket);
        exit(-1);
    }

    svMessage[filenameLen] = '\0';
    if (!strcmp(svMessage, "exist")) {
        printf("This file doesn\'t exist.\n");
        free(svMessage);
        return;
    } else if (strcmp(svMessage, "confirmed")) {
        printf("Something went wrong.\n");
        free(svMessage);
        close(socket);
        exit(-1);
    }
    free(svMessage);

    // Receive file size
    ssize_t fileSize = 0;
    if (recv(socket, &fileSize, sizeof(ssize_t), 0) <= 0) {
        printf("\nServer closed.\n");
        close(socket);
        exit(0);
    }

    // Create "files" folder if it doesn't exists
    struct stat st;
    if (stat("files", &st)) {
        if (mkdir("files", 0777)) {
            printf("Failed to create \"files/\" directory.\n");
            close(socket);
            exit(-1);
        }
    }

    // Open the file for writing
    char* filePath = (char*)malloc(strlen(filename) + 7);
    if (filePath == NULL) {
        printf("Error allocating memory for server response.\n");
        close(socket);
        exit(-1);
    }

    snprintf(filePath, strlen(filename) + 7, "%s/%s", "files", filename);
    int fileDescriptor = open(filePath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fileDescriptor < 0) {
        printf("Error opening the file descriptor.\n");
        free(filePath);
        close(socket);
        exit(-1);
    }

    // Receive and write the file content
    char buffer[1024];
    ssize_t bytesRead;
    ssize_t totalBytesRead = 0;

    while (totalBytesRead != fileSize) {
        if ((bytesRead = recv(socket, buffer, 1024, 0)) <= 0) {
            printf("\nServer closed.\n");
            close(fileDescriptor);
            close(socket);
            remove(filePath);
            free(filePath);
            exit(0);
        }
        if (write(fileDescriptor, buffer, bytesRead) <= 0) {
            printf("Failed to write to file.\n");
            close(fileDescriptor);
            close(socket);
            remove(filePath);
            free(filePath);
            exit(0);
        }
        totalBytesRead += bytesRead;
    }

    if (totalBytesRead == fileSize) {
        printf("File downloaded successfully!\n");
    } else {
        printf("Failed to download the file.\n");
        close(fileDescriptor);
        remove(filePath);
        free(filePath);
        return;
    }

    close(fileDescriptor);
    free(filePath);
}

void deleteFile(int socket, char* filename) {
    // Send filename to the server
    ssize_t msgLen = strlen(filename);
    send(socket, &msgLen, sizeof(ssize_t), 0);
    send(socket, filename, msgLen, 0);

    msgLen = 0;
    if (recv(socket, &msgLen, sizeof(ssize_t), 0) <= 0) {
        printf("\nServer closed.\n");
        close(socket);
        exit(0);
    }

    char* data = (char*)malloc(msgLen + 1);
    if (data == NULL) {
        printf("Failed to allocate memory.\n");
        close(socket);
        exit(-1);
    }

    if (recv(socket, data, msgLen, 0) <= 0) {
        printf("\nServer closed.\n");
        close(socket);
        free(data);
        exit(0);
    }
    data[msgLen] = '\0';

    if (!strcmp(data, "end")) {
        printf("File deleted successfully!\n");
    } else if (!strcmp(data, "nfnd")) {
        printf("Couldn\'t find %s\n", filename);
    } else if (!strcmp(data, "error")) {
        printf("Error trying to delete the file.\n");
    }

    free(data);
}
