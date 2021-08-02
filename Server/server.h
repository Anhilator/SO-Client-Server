#pragma once
#include "linked_list.h"
# include "private_chat_struct.h"

void* connection_handler(int socket_desc);

void printTitle();

// operazioni che il client può richiedere
void authentication(char buf[], int recv_bytes, struct sockaddr_in client_addr, int sockaddr_len);
void logout(struct sockaddr_in client_addr, int sockaddr_len);
void signin(char buf[], int recv_butes, struct sockaddr_in client_addr, int sockaddr_len);
void show_chat(struct sockaddr_in client_addr, int sockaddr_len);
void show_messages(char buf[], int recv_bytes, struct sockaddr_in client_addr, int sockaddr_len);
void get_chat(char buf[],ListHead chats);
void getMessages(char buf[],ListHead messages);
void new_chat(char buf[], int recv_bytes, struct sockaddr_in client_addr, int sockaddr_len);


int file_authentication(char * username, char * password);

void startServer();

void chooseOperation(char buf[], int recv_bytes, struct sockaddr_in client_addr, int sockaddr_len);
void sendRespone(char buf[], struct sockaddr_in client_addr, int sockaddr_len);

int fileRead_Database(Database* database);
int readFile_Chats(User* user, FILE* chats);

// operazioni sulle strutture dati
User* User_findByUsername(ListHead* head, const char* username);
LoginListItem* LoginListItem_findBySockaddr_in(ListHead* head, struct sockaddr_in client_addr, int sockaddr_len);
ChatListItem* ChatListItem_findByUser(ListHead* head, User* receiver);
ChatListItem* ChatListItem_findByUsers(User* sender, User* receiver);

User* initUser(char username[], char password[]);
ChatListItem** initChat(User* sender, User* receiver);

void setupDatabase();

void addNewLogin(User* user,struct sockaddr_in client_addr, int sockaddr_len);
int Database_readOneMessage(ChatListItem* chat, FILE* file_chat);

