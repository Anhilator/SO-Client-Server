

#define MAX_USER_LEN 20
#define MAX_PASSWORD_LEN 20
#define MAX_OPERATION_LEN 6
#define MAX_MESSAGE_SIZE 1024


typedef struct Chat{
    char* username2;
    int num_messages_w2;
    char** messages;
}Chat;

typedef struct User{
    char* username;
    Chat* chat;
    int num_chat;
    int logged;
}User;

void login(int socket_desc, struct sockaddr_in server_addr,User* user);
void signin(int socket_desc,struct sockaddr_in server_addr);
void logout(int socket_desc,struct sockaddr_in server_addr,User* user);
void get_chat(int socket_desc,struct sockaddr_in server_addr,User* user);

void init_user(User* user, char* username, int logged,int socket_desc,struct sockaddr_in server_addr);
void destroy_user(User* user);
void show_chat(User* user);
void init_chat(Chat* chat,char* chat_string);
