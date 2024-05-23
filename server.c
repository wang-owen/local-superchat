#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h> 
#include <semaphore.h> 

#include "chat.h"

void* user_thread(void*);

int main() {
    struct addrinfo hints, * res, * p;
    int status;
    int serverfd;
    int clientfd;
    char active;
    struct sockaddr_storage clientaddrs;
    socklen_t sin_size;
    int bytesrecv;
    char username[MAX_USERNAME_LENGTH];
    pthread_t user_threads[MAX_USERS];
    int num_users;


    // Initialize server
    memset(&hints, 0, sizeof hints);
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(NULL, SERVER_PORT, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }

    for (p = res; p != NULL; p = p->ai_next) {
        if ((serverfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            continue;
        }
        if ((bind(serverfd, p->ai_addr, p->ai_addrlen)) == -1) {
            close(serverfd);
            continue;
        }
        break;
    }
    if (p == NULL) {
        fprintf(stderr, "SERVER: Failed to bind\n");
        return 1;
    }

    // Begin to accept client connections
    if ((listen(serverfd, MAX_USERS)) == -1) {
        perror("SERVER: listen");
        return 1;
    }
    num_users = 0;

    active = 1;
    while (active) {
        // Accept client connection
        if ((clientfd = (accept(serverfd, (struct sockaddr*)&clientaddrs, &sin_size))) == -1) {
            perror("SERVER: accept");
            break;
        }

        // Send success message to client
        char* msg = "Connected to server.";
        if ((send(clientfd, msg, strlen(msg), 0)) == -1) {
            perror("SERVER: send");
            break;
        }

        // Get username from client
        if ((bytesrecv = recv(clientfd, username, sizeof username, 0)) == 0) {
            fprintf(stderr, "SERVER: recv");
            return 1;
        }
        username[bytesrecv] = 0;

        // Create thread for client
        if ((pthread_create(&user_threads[num_users], NULL, user_thread, &clientfd)) != 0) {
            fprintf(stderr, "SERVER: Failed to create thread\n");
            return 1;
        }

        num_users++;
        printf("%s connected.\n", username);
    }
    close(serverfd);
}

void* user_thread(void* arg) {
    int clientfd = *((int*)arg);
    char msg[MAX_MESSAGE_LENGTH];
    int bytesrecv;
    while (1) {
        if ((bytesrecv = recv(clientfd, msg, sizeof msg, 0)) == -1) {
            perror("SERVER: send");
            break;
        }
        else if (bytesrecv == 0) {
            printf("Client disconnected.\n");
            break;
        }
        msg[bytesrecv] = 0;
        printf("%s\n", msg);
    }

    return NULL;
}