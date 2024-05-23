#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include "chat.h"

int main() {
    struct addrinfo hints, * res, * p;
    int sockfd;
    int status;
    char username[MAX_USERNAME_LENGTH];
    int bytesrecv;
    char msg[MAX_MESSAGE_LENGTH];

    // Get username
    printf("Username (max %d): ", MAX_USERNAME_LENGTH);
    scanf("%s", username);

    // Initalize client
    memset(&hints, 0, sizeof hints);
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(NULL, SERVER_PORT, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }

    for (p = res; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            continue;
        }
        if ((connect(sockfd, p->ai_addr, p->ai_addrlen)) == -1) {
            continue;
            close(sockfd);
        }
        break;
    }
    if (p == NULL) {
        fprintf(stderr, ("CLIENT: Failed to connect to server\n"));
        return 1;
    }

    // Send username to server
    if ((send(sockfd, username, strlen(username), 0)) == -1) {
        perror("CLIENT: send");
    }

    while (1) {
        // Send message to server
        printf("> ");
        fgets(msg, sizeof msg, stdin);
        msg[strlen(msg) - 1] = 0;
        if ((send(sockfd, msg, strlen(msg), 0)) == -1) {
            perror("CLIENT: send");
            break;
        }
    }

    close(sockfd);
    return 0;
}