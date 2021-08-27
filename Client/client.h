
#include "linked_list.h"
#define USER_LEN 20
#define PASS_LEN 20
#define CMD_LEN 44
#define OP_LEN 6
#define RESP_LEN 1024
#define MAX_MSG_SIZE 10000

/*Una chat viene vista come una linked list.
Ogni chat contiene due informazioni l'username dell'altro utente (other_user e'
un char* poiche' come client io devo avere a mia disposizione solo i dati degli utenti loggati)
e il numero di messaggi che contiene la lista.*/
typedef struct ChatListItem{

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

void login();
void signin();
void logout();
void show_chat();
void show_messages(ChatListItem* chat);
void new_chat(char* other_user);

void init_user(char* username, int logged);
void destroy_user();

ChatListItem* get_messages(char* user2);
void get_all_chats();
ChatListItem* find_chat_by_other_user(char* user2);

void clientTerminationHandler(int signum);