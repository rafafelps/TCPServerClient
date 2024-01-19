#include "tcpnet.h"

#define AUTH "ohygIf2YICKdNafb5YePqgI02EuI6Cd"
#define COMM_LEN 64

struct Client {
    int socket;
    uint8_t isRunning;
};

void* clientConsole(void* cl);

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
    } else if (!strcmp(svMessage, "full")) {
        printf("Server is full.\n");
        close(client.socket);
        return 0;
    } else if (!strcmp(svMessage, "auth")) {
        printf("Wrong authentication code.\n");
        close(client.socket);
        return 0;
    }

    // Spawn client console thread
    client.isRunning = 1;
    pthread_t consoleThread;
    pthread_create(&consoleThread, NULL, clientConsole, &client);

    // Create a new socket descriptor for non-blocking recv
    int nonBlockingSocket = dup(client.socket);

    // Set the new socket descriptor to non-blocking mode
    int flags = fcntl(nonBlockingSocket, F_GETFL, 0);
    fcntl(nonBlockingSocket, F_SETFL, flags | O_NONBLOCK);

    // Verify for a closed server
    while (strcmp(svMessage, "clsd") && client.isRunning) {
        recv(nonBlockingSocket, svMessage, 4, 0);

        // Limit loop to 4 times a second | 250,000 microseconds
        usleep(250000);
    }

    if (client.isRunning) {
        printf("\nServer closed.\n");
    }


    #ifdef MESSAGEHNDL
    char userInput[10] = {'\0'};
    printf(">");
    scanf("%s", userInput);

    if (!strcmp(userInput, "ls")) {
        struct stat st;
        if (stat("files", &st)) {
            if (mkdir("files", 0777)) {
                printf("Failed to create \"files/\" directory.\n");
                return -1;
            }
        }

        DIR* dir = opendir("files");
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                // Print the name of each file in the directory
                printf("%s\n", entry->d_name);
            }
        }
        closedir(dir);
    }

    // Recieve data from server
    char serverResponse[256];
    if (recv(clientSocket, &serverResponse, sizeof(serverResponse), 0) == -1) {
        printf("Error receiving server data.\n");
        close(clientSocket);
        return -1;
    }

    printf("Server sent the data: %s\n", serverResponse);
    #endif

    close(nonBlockingSocket);
    close(client.socket);
    return 0;
}

void* clientConsole(void* cl) {
    struct Client* client = (struct Client*)cl;

    char command[COMM_LEN] = {'\0'};
    while (client->isRunning) {
        printf("\n> ");
        fgets(command, sizeof(command), stdin);

        // Lowercase first command
        char action[COMM_LEN] = {'\0'};
        uint32_t wordEnd;
        for (wordEnd = 0; command[wordEnd] != '\0'; wordEnd++) {
            if (command[wordEnd] == ' ' || command[wordEnd] == '\n') { break; }

            action[wordEnd] = tolower(command[wordEnd]);
        }

        if (!strcmp(action, "list") || !strcmp(action, "ls")) {
            // List command
            send(client->socket, "list", strlen("list"), 0);
        } else if (!strcmp(action, "upload") || !strcmp(action, "up")) {
            // Upload command
            send(client->socket, "upld", strlen("upld"), 0);
        } else if (!strcmp(action, "download") || !strcmp(action, "dwn")) {
            // Download command
            send(client->socket, "down", strlen("down"), 0);
        } else if (!strcmp(action, "delete") || !strcmp(action, "del")) {
            // Delete command
            send(client->socket, "dlfl", strlen("dlfl"), 0);
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
            client->isRunning = 0;
        } else {
            printf("Command not found. Type \"help\" or \"h\" for help.\n");
        }
    }

    return NULL;
}
