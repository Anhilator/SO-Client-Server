typedef struct Chat{
    char* username2;
    int num_messages_w2;
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
void show_chat(int socket_desc,struct sockaddr_in server_addr,User* user);

