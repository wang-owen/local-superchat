#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <ifaddrs.h>
#include <netinet/in.h>

#include "chat.h"


void* get_messages(void*);
int get_local_ip(char*, char*);

int main() {
    struct addrinfo hints, * res, * p;
    int sockfd;
    int status;
    char username[MAX_USERNAME_LENGTH];
    pthread_t thread_id;
    int bytesrecv;
    char msg[MAX_MESSAGE_LENGTH];
    char ip[INET6_ADDRSTRLEN];

    // Get username
    printf("Username (max %d): ", MAX_USERNAME_LENGTH);
    scanf("%s", username);

    // Initalize client
    memset(&hints, 0, sizeof hints);
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (get_local_ip("en0", ip) == -1) {
        return 1;
    }

    if ((status = getaddrinfo(ip, SERVER_PORT, &hints, &res)) != 0) {
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

    // Create thread to receive messages
    if ((pthread_create(&thread_id, NULL, get_messages, &sockfd)) != 0) {
        fprintf(stderr, "SERVER: Failed to create thread\n");
        return 1;
    }

    while (1) {
        // Send message to server
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

void* get_messages(void* arg) {
    int sockfd = *(int*)arg;
    int bytesrecv;
    char msg[MAX_MESSAGE_LENGTH];
    while (1) {
        if ((bytesrecv = recv(sockfd, msg, sizeof msg, 0)) == -1) {
            perror("CLIENT: recv");
            break;
        }
        else if (bytesrecv == 0) {
            fprintf(stderr, "Server has disconnected.\n");
            exit(1);
        }
        msg[bytesrecv] = 0;
        printf("%s\n", msg);
    }
    return NULL;
}

int get_local_ip(char* interface, char* ipstr) {
    struct ifaddrs* ifaddr, * ifa;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return -1;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        // Check if the interface name matches
        if (strcmp(ifa->ifa_name, interface) != 0)
            continue;

        int family = ifa->ifa_addr->sa_family;

        // Only consider IPv4 and IPv6 addresses
        if (family == AF_INET || family == AF_INET6) {
            int s = getnameinfo(ifa->ifa_addr,
                (family == AF_INET) ? sizeof(struct sockaddr_in) :
                sizeof(struct sockaddr_in6),
                host, NI_MAXHOST,
                NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                continue;
            }

            strcpy(ipstr, host);
            break;
        }
    }
    freeifaddrs(ifaddr);
    return 0;
}