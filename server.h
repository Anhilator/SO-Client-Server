#pragma once
#include "linked_list.h"

void* connection_handler(int socket_desc);

void printTitle();

void authentication(char buf[], int recv_bytes, struct sockaddr_in client_addr, int sockaddr_len);

int file_authentication(char * username, char * password);

void startServer();

void chooseOperation(char buf[], int recv_bytes, struct sockaddr_in client_addr, int sockaddr_len);
void sendRespone(char buf[], int bytes_left, struct sockaddr_in client_addr, int sockaddr_len);