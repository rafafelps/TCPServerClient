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
            // Upload command
            send(client->socket, "upld", strlen("upld"), 0);
        } else if (!strcmp(action, "download") || !strcmp(action, "dwn")) {
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
