
#include "linked_list.h"
#define MAX_USER_LEN 20
#define MAX_PASSWORD_LEN 20
#define MAX_OPERATION_LEN 6
#define MAX_MESSAGE_SIZE 1024

/*Un messaggio viene visto come una linked list di stringhe.*/
typedef struct MessageListITem{
    ListItem item;
    char* message;
}MessageListItem;

/*Una chat viene vista come una linked list.
Ogni chat contiene due informazioni l'username dell'altro utente (other_user e'
un char* poiche' come client io devo avere a mia disposizione solo i dati degli utenti loggati)
e il numero di messaggi che contiene la lista.*/
typedef struct ChatListItem{
    ListHead* messages;
    ListItem item;
    char* other_user;
    int num_messages;

}ChatListItem;
/*Un utente viene inizializzato nella fase di login, dell'utente non manteniamo nessuna 
informazione importante se non il suo nome utente, utile solo a fini di GUI, e il numero di chat che
ha attualmente attive.*/
typedef struct User{
    ListHead* chats;
    char* username;
    int num_chat;
    int logged;
}User;

void login(int socket_desc, struct sockaddr_in server_addr,User* user);
void signin(int socket_desc,struct sockaddr_in server_addr);
void logout(int socket_desc,struct sockaddr_in server_addr,User* user);
void show_chat(User* user);
void show_messages(ChatListItem* chat);

void init_user(User* user, char* username, int logged,int socket_desc,struct sockaddr_in server_addr);
void destroy_user(User* user);

ChatListItem* get_messages(int socket_desc, struct sockaddr_in server_addr,User* user,char* user2);
void get_all_chats(int socket_desc,struct sockaddr_in server_addr,User* user);
ChatListItem* find_chat_by_other_user(User* user, char* user2);