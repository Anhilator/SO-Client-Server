#pragma once
#include "linked_list.h"
//#include "private_chat_constants.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>


// struct per contenere un messaggio generico
typedef struct Message{
    char message[100];
    int len_message;
    int time[6]; //year, month, day, hour, minute ,seconds

} Message;

// list per contenere un certo numero di messaggi
typedef struct MessageListItem{ 
    ListItem list;
    Message* message;
    char sent;               // true se mittente è il detentore della chat
} MessageListItem;

// struct per rappresentare un utente (contiene le sue chat, credenziali e altri dati)
typedef struct User { 
    ListItem list;
    char username[50];
    char password[50]; 
    ListHead chats;
    int num_chats;
    int logged;
} User;

// struttura per rappresentare la chat di un utente a verso un utente b
// n.b. se esiste un oggetto chat da a verso b, esisterà anche il corrispettivo da b verso a
typedef struct ChatListItem {
    ListItem list;
    ListHead messages;
    ListHead messages_unread;
    User* sender;
    User* receiver;
    struct ChatListItem* symmetrical_chat;
    int num_messages;
    int num_messages_unread;
} ChatListItem;

// struct per rappresentare il login di un utente, utile da avere nel databse per sapere quali sono gli utenti loggati
typedef struct LoginListItem{
    ListItem list;
    User* user;
    struct sockaddr_in client_addr;
    int sockaddr_len;
} LoginListItem;

// struct per immagazzinare un certo numero di utenti e quanti di questi utenti si sono attualmente loggati
typedef struct Database {
    ListHead users;
    ListHead login; //lista degli utenti loggati
    int num_users;
} Database;


User* User_findByUsername(ListHead* head, const char* username);

ChatListItem* ChatListItem_findByUser(ListHead* head, User* receiver);