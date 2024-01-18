#include "tcpnet.h"

#define AUTH "ohygIf2YICKdNafb5YePqgI02EuI6Cd"

int main(int argc, char* argv[]) {
    if (!isValidClientCommand(argc, argv)) { return -1; }

    // Creates socket and sockaddr
    int clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket < 0) { return -1; }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &serverAddress.sin_addr);

    printf("Attempting to connect to %s:%s...\n", argv[1], argv[2]);
    if (connectClient(clientSocket, &serverAddress) == -1) { return -1;}
    send(clientSocket, AUTH, strlen(AUTH), 0);
    char svMessage[5] = {'\0'};
    recv(clientSocket, svMessage, 4, 0);
    if (!strcmp(svMessage, "conn")) {
        printf("Connected to the server!\n");
    }

    char userInput[10] = {'\0'};
    printf(">");
    scanf("%s", userInput);

    if (!strcmp(userInput, "ls")) {
        struct stat st;
        if (stat("files", &st)) {
            if (mkdir("files", 0777)) {
                printf("Failed to create files directory.\n");
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

    #ifdef MESSAGEHNDL
    // Recieve data from server
    char serverResponse[256];
    if (recv(clientSocket, &serverResponse, sizeof(serverResponse), 0) == -1) {
        printf("Error receiving server data.\n");
        close(clientSocket);
        return -1;
    }

    printf("Server sent the data: %s\n", serverResponse);
    #endif

    close(clientSocket);
    return 0;
}