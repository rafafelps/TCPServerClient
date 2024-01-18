#include "tcpnet.h"

int isValidClientCommand(int argc, char* argv[]) {
    // Check arguments length
    if (argc != 3) {
        char* program = strrchr(argv[0], '\\');

        if (program == NULL) {
            program = argv[0];
            printf("Error. Command usage: %s [ip] [port]\n", program);
        } else {
            printf("Error. Command usage: .%s [ip] [port]\n", program);
        }
        
        return 0;
    }

    // Check for valid ip format
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, argv[1], &(sa.sin_addr));
    if (!result) {
        printf("Error. Wrong ip format.\n");
        return 0;
    }

    // Check for valid port format
    for (int i = 0; argv[2][i] != '\n' && argv[2][i] != '\0'; i++) {
        if (argv[2][i] < '0' || argv[2][i] > '9') {
            printf("Error. Wrong port format.\n");
            return 0;
        }
    }
    result = atoi(argv[2]);
    if (result < 0 || result > 65535) {
        printf("Error. Wrong port format.\n");
        return 0;
    }

    return 1;
}

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

int connectClient(int clientSocket, struct sockaddr_in* serverAddress) {
    if (connect(clientSocket, (struct sockaddr*)serverAddress, sizeof(*serverAddress)) < 0) {
        printf("Couldn\'t reach the server.\n");
        close(clientSocket);
        return -1;
    }
    return 0;
}
