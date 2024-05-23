#define SERVER_PORT "3000"
#define MAX_USERS 10
#define MAX_USERNAME_LENGTH 25
#define MAX_MESSAGE_LENGTH 100

struct user_data {
    char username[MAX_USERNAME_LENGTH];
    int clientfd;
};