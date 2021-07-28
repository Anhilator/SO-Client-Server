#pragma once
#include "linked_list.h"
# include "private_chat.h"

void* connection_handler(int socket_desc);

void printTitle();

// operazioni che il client può richiedere
void authentication(char buf[], int recv_bytes, struct sockaddr_in client_addr, int sockaddr_len);
void logout(struct sockaddr_in client_addr, int sockaddr_len);

int file_authentication(char * username, char * password);

void startServer();

void chooseOperation(char buf[], int recv_bytes, struct sockaddr_in client_addr, int sockaddr_len);
void sendRespone(char buf[], struct sockaddr_in client_addr, int sockaddr_len);

int fileRead_Database(Database* database);

// operazioni sulle strutture dati
User* User_findByUsername(ListHead* head, const char* username);
LoginListItem* LoginListItem_findBySockaddr_in(ListHead* head, struct sockaddr_in client_addr, int sockaddr_len);

User* initUser(char username[], char password[]);

void setupDatabase();

void addNewLogin(User* user,struct sockaddr_in client_addr, int sockaddr_len);

