#include "tcpnet.h"

int isValidCommand(int argc, char* argv[]) {
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
