#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h> 
#include <ctype.h>

#include "chat.h"

int num_users;
int clientfds[MAX_USERS];

void* user_thread(void*);
void* clean_username(void*);

int main() {
    struct addrinfo hints, * res, * p;
    int status;
    int serverfd;
    int clientfd;
    int yes = 1;
    char active;
    struct sockaddr_storage clientaddrs;
    socklen_t sin_size;
    int bytesrecv;
    char accepted;
    char username[MAX_USERNAME_LENGTH];
    char usernames[MAX_USERS][MAX_USERNAME_LENGTH];
    struct user_data data;
    pthread_t user_threads[MAX_USERS];


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
        if (setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            return 1;
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
        accepted = 1;
        if ((clientfd = (accept(serverfd, (struct sockaddr*)&clientaddrs, &sin_size))) == -1) {
            perror("SERVER: accept");
            break;
        }

        // Get username from client
        if ((bytesrecv = recv(clientfd, username, sizeof username, 0)) == -1) {
            perror("SERVER: recv");
            return 1;
        }
        else if (bytesrecv == 0) {
            fprintf(stderr, "Client has disconnected.");
            return 1;
        }
        username[bytesrecv] = 0;

        // Make sure username is not a duplicate
        clean_username(username);
        int err = 0;
        if (num_users > 0) {
            for (int i = 0; i < num_users; i++) {
                if (strcmp(username, usernames[i]) == 0) {
                    err = -1;
                    accepted = 0;
                    break;
                }
            }
        }
        if ((send(clientfd, &err, sizeof err, 0)) == -1) {
            perror("SERVER: send");
            break;
        }

        if (accepted) {
            strcpy(usernames[num_users], username);
            // Send success message to client
            char* msg = "Connected to server.";
            if ((send(clientfd, msg, strlen(msg), 0)) == -1) {
                perror("SERVER: send");
                break;
            }

            // Create thread for client
            strcpy(data.username, username);
            data.clientfd = clientfd;
            if ((pthread_create(&user_threads[num_users], NULL, user_thread, &data)) != 0) {
                fprintf(stderr, "SERVER: Failed to create thread\n");
                return 1;
            }

            clientfds[num_users] = clientfd;
            num_users++;
            printf("%s connected.\n", username);
        }
    }
    close(serverfd);
}

void* user_thread(void* arg) {
    struct user_data data = *(struct user_data*)arg;
    char msg[MAX_MESSAGE_LENGTH];
    char sendmsg[MAX_MESSAGE_LENGTH + MAX_USERNAME_LENGTH + 2];
    int bytesrecv;

    while (1) {
        // Receive messages
        if ((bytesrecv = recv(data.clientfd, msg, sizeof msg, 0)) == -1) {
            perror("SERVER: send");
            break;
        }
        else if (bytesrecv == 0) {
            printf("Client disconnected.\n");
            break;
        }
        msg[bytesrecv] = 0;

        // Send messages to all connected clients
        sprintf(sendmsg, "%s: %s", data.username, msg);
        printf("%s\n", sendmsg);
        for (int i = 0; i < num_users; i++) {
            if (clientfds[i] != data.clientfd && (send(clientfds[i], sendmsg, strlen(sendmsg), 0) == -1)) {
                perror("SERVER: send");
            }
        }
    }
    return NULL;
}

void* clean_username(void* arg) {
    char* username = (char*)arg;
    while (*username) {
        *username = tolower((unsigned char)*username);
        username++;
    }
    return NULL;
}