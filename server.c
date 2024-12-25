#include "common.h"

int num_users, max_users = 10;
int *clientfds;
char **usernames;

void *user_thread(void *);
int clean_username(char *);

struct user_data {
  int clientfd;
  char username[MAX_USERNAME_LENGTH];
  int user_pos;
};

void *user_thread(void *arg) {
  struct user_data data = *(struct user_data *)arg;
  char msg[MAX_MESSAGE_LENGTH];
  char sendmsg[MAX_MESSAGE_LENGTH + MAX_USERNAME_LENGTH + 2];
  int bytesrecv;

  while (1) {
    // Receive messages
    if ((bytesrecv = recv(data.clientfd, msg, sizeof msg, 0)) == -1) {
      perror("SERVER: send");
      break;
    } else if (bytesrecv == 0) {
      printf("Client disconnected.\n");
      // Free up username
      strcpy(usernames[data.user_pos], "?");
      break;
    }
    msg[bytesrecv] = 0;

    // Send messages to all connected clients
    sprintf(sendmsg, "%s: %s", data.username, msg);
    printf("%s\n", sendmsg);
    for (int i = 0; i < num_users; i++) {
      if (clientfds[i] != data.clientfd &&
          (send(clientfds[i], sendmsg, strlen(sendmsg), 0) == -1)) {
        perror("SERVER: send");
      }
    }
  }
  return NULL;
}

int clean_username(char *username) {
  while (*username) {
    if (*username >= 48 && *username <= 57     // 0-9
        || *username >= 65 && *username <= 90  // A-Z
        || *username >= 97 && *username <= 122 // a-z
        || *username == 95) {                  // _
      if (*username >= 65 && *username <= 90) {
        *username = *username + 32;
      }
      username++;
    } else {
      return -1;
    }
  }
  return 0;
}

int main() {
  int status, serverfd, clientfd, bytesrecv;
  char active;
  char username[MAX_USERNAME_LENGTH];
  struct addrinfo hints, *res, *p;
  struct sockaddr_storage clientaddrs;
  socklen_t sin_size;
  struct user_data data;
  pthread_t tid;

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
    if ((serverfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) ==
        -1) {
      continue;
    }
    // Force on SERVER_PORT
    int yes = 1;
    if (setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) ==
        -1) {
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

  // Allocate usernames array
  usernames = (char **)malloc(max_users * sizeof(char *));
  for (int i = 0; i < max_users; i++) {
    usernames[i] = (char *)malloc(MAX_USERNAME_LENGTH * sizeof(char));
  }

  num_users = 0;
  active = 1;

  // Allocate clientfds array
  clientfds = (int *)malloc(max_users * sizeof(int));
  if (clientfds == NULL) {
    fprintf(stderr, "SERVER: Memory allocation error\n");
    return 1;
  }

  // Listen for client connections
  if ((listen(serverfd, max_users)) == -1) {
    perror("SERVER: listen");
    return 1;
  }
  while (active) {
    // Accept client connection
    if ((clientfd = (accept(serverfd, (struct sockaddr *)&clientaddrs,
                            &sin_size))) == -1) {
      perror("SERVER: accept");
      break;
    }

    // Get username from client
    if ((bytesrecv = recv(clientfd, username, sizeof username, 0)) == -1) {
      perror("SERVER: recv");
      return 1;
    }
    username[bytesrecv] = 0;

    // Check username
    int err = 0;
    // Check if legal format
    if (clean_username(username) == -1) {
      err = -1;
    }
    // Check for duplicates
    else if (num_users > 0) {
      for (int i = 0; i < num_users; i++) {
        if (strcmp(username, usernames[i]) == 0) {
          err = -2;
          break;
        }
      }
    }
    if ((send(clientfd, &err, sizeof err, 0)) == -1) {
      perror("SERVER: send");
      break;
    }

    if (err == 0) {
      if (num_users + 1 > max_users) {
        int new_size = max_users + 10;
        // Resize clientfds
        clientfds = realloc(clientfds, new_size * sizeof(int));

        // Resize usernames
        usernames = realloc(usernames, new_size * sizeof(char *));
        for (int i = num_users; i < new_size; i++) {
          usernames[i] = (char *)malloc(MAX_USERNAME_LENGTH * sizeof(char));
        }
      }

      strcpy(usernames[num_users], username); // Save username

      // Send success message to client
      char *msg = "Connected to server.";
      if ((send(clientfd, msg, strlen(msg), 0)) == -1) {
        perror("SERVER: send");
        break;
      }

      // Create thread for client
      strcpy(data.username, username);
      data.clientfd = clientfd;
      data.user_pos = num_users;
      if ((pthread_create(&tid, NULL, user_thread, &data)) != 0) {
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
