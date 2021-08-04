#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>  // htons() and inet_addr()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
#include <termios.h>
#include "common.h"
#include "../CSP.h"
#include "client.h"
#include "functions.c"

void init_chat_from_str(Chat* chat,char* chat_string,int index){
    char* tok  = strtok(chat_string,"**");

    chat[index].username2 = malloc(sizeof(char*) * strlen(tok) + 1);
    strcpy(chat[index].username2,tok);
    chat[index].num_messages_w2 = atoi(strtok(NULL,"**"));
    chat[index].messages = (char**) malloc(sizeof(char*) * chat->num_messages_w2);

}

void init_user(User* user, char* username, int logged,int socket_desc,struct sockaddr_in server_addr){
    user->username = (char*)malloc(sizeof(char*) * strlen(username)+1);
    strcpy(user->username, username);
    user->logged = logged;
    get_chat(socket_desc,server_addr,user);
}

void destroy_user(User* user){
    free(user->chat);
    free(user->username);
    
    user->num_chat = 0;
    user->logged = 0;
}



/*Sequenza di login, logged viene passato come argomento perche' il client potrebbe essere crashato o
interrotto durante una precedente esecuzione, tale variabile permette di "interrogare" il database 
sull'effettivo stato dell'utente.*/
void login(int socket_desc, struct sockaddr_in server_addr,User* user){
    char log_cmd[44],password[20],username[20],server_response[1024];
    int log_len;
    while (1) {
        if(DEBUG) {
            sender("1::Samuele::password1",44,socket_desc,server_addr);
            reciever(server_response,sizeof(server_response), socket_desc);

            init_user(user, "Samuele",1,socket_desc,server_addr);
            get_chat(socket_desc,server_addr,user);
            return;
        }
        printf("Enter username: ");
        reader(username,MAX_USER_LEN,"utente");        

        printf("Enter password: ");
        reader_password(password,MAX_PASSWORD_LEN);

        generate_command(username,password,log_cmd,1);

        log_len = strlen(log_cmd);
        sender(log_cmd,log_len,socket_desc,server_addr);
        reciever(server_response,sizeof(server_response), socket_desc);

        char success[100];
        sprintf(success,LOGIN_SUCCESS,username); // necessario per una corretta formattazione
   
        if(!strcmp(LOGIN_SUCCESS, server_response) || !strcmp(ALREADY_LOGGED, server_response)) {
            printTitle();
            if(!strcmp(ALREADY_LOGGED, server_response)) printf("%s\n", ALREADY_LOGGED);
            printf("Bentornato nel server, %s.\n", username);
            init_user(user, username,1,socket_desc,server_addr);
            print_user(user,0);

            return;
        }
        else{
            printTitle();
            printf("É stata inserito un username e/o password sbagliata, riprovare\n");
        }
    }
}
void signin(int socket_desc,struct sockaddr_in server_addr){
    char sign_cmd[44],server_response[1024],username[20],password[20];
    int sign_len;
    while(1){

        printf("Enter the username you'll be using: ");
        reader(username,MAX_USER_LEN,"utente");        

        printf("Enter password you'll be using: ");
        reader_password(password,MAX_PASSWORD_LEN);


        generate_command(username,password,sign_cmd,2);

        sign_len = strlen(sign_cmd);
        sender(sign_cmd,sign_len,socket_desc,server_addr);
        reciever(server_response,sizeof(server_response), socket_desc);

        if(!strcmp(SIGNIN_SUCCESS, server_response)) {
            printTitle();
            printf("Benvenuto/a nel server, %s.\n",username);
            return;
        }
        else{
            printTitle();
            printf("%s é già presente nel server\n",username);
            return;
        }
    }
}
void logout(int socket_desc,struct sockaddr_in server_addr,User* user){
    char cmd[10];
    generate_command(NULL, NULL, cmd, 4);
    sender(cmd,sizeof(cmd),socket_desc,server_addr);
    char server_response[100];
    reciever(server_response, sizeof(server_response), socket_desc);
    if(!strcmp(server_response,LOGOUT_SUCCESS)) destroy_user(user);

}
void get_chat(int socket_desc,struct sockaddr_in server_addr,User* user){    
    char cmd[10],server_response[100];
    generate_command(NULL, NULL, cmd,3);
    sender(cmd,sizeof(cmd),socket_desc,server_addr);
    reciever(server_response,sizeof(server_response),socket_desc);
    char* tok = strtok(server_response,"::");
    user->num_chat = atoi(tok);
    user->chat = (Chat*) malloc(sizeof(Chat) * user->num_chat);
    char** tmp = (char**) malloc(sizeof(char*) * user->num_chat);
    for(int i =0;i< user->num_chat;i++){
        tok = strtok(NULL,"::");
        if(tok!=NULL) {
            tmp[i] =(char*) malloc(sizeof(char)* strlen(tok));
            strcpy(tmp[i],tok);
        }

    }
    for(int i =0;i< user->num_chat;i++){
        init_chat_from_str(user->chat,tmp[i],i);
    }
}

void show_chat(User* user){
    if(user != NULL){
        printf("L'utente %s, ha %d chat%s",user->username, user->num_chat,(user->num_chat>0)?" con:\n":"\n");
        if(user->num_chat <= 0) {
            printf("\n");
            return;
        }
        for (int i =0; i < user->num_chat;i++){
            printf("\tCon %s ha %d chat.\n",user->chat[i].username2,user->chat->num_messages_w2);  
        } 
    }
}

int find_chat_by_other_user(User* user, char* user2){
    for(int i =0; i < user->num_chat;i++){
        if(!strcmp(user->chat[i].username2,user2)) return i;
    }
    return -1;
}
void show_messages(int socket_desc, struct sockaddr_in server_addr,User* user){
    get_chat(socket_desc,server_addr,user);
    if(user->num_chat <= 0) {
        printf("l'utente non ha chat attualmente.\n");
        return;
    }
    char msg_cmd[44],username2[20],server_response[1024];   
    int msg_len,index;
    
        printf("Enter username of the user view the chats with: ");
        reader(username2,MAX_USER_LEN,"utente");        

        generate_command(username2,NULL,msg_cmd,5);
        index = find_chat_by_other_user(user, username2);
        if(index < 0){
            printf("Non si hanno chat con questo utente\n");
            return;
        }
        msg_len = strlen(msg_cmd);
        sender(msg_cmd,msg_len,socket_desc,server_addr);
        reciever(server_response,sizeof(server_response), socket_desc);
        char* tok = strtok(server_response, "\n");
        for(int i = 0; i < user->chat[index].num_messages_w2;i++){
            user->chat[index].messages[i] = (char*) malloc(sizeof(char) * strlen(tok));
            user->chat[index].messages[i] = tok;
            tok= strtok(NULL,"\n");
        }
        free(tok);
        for(int i = 0; i < user->chat[index].num_messages_w2;i++){
            
            char* tok = strtok(user->chat[index].messages[i],"::");
            char* sender;
            if(*tok == 's'){
                sender = malloc(sizeof(char) * strlen(user->username));
                strcpy(sender,user->username);
            }
            else {
                sender = malloc(sizeof(char) * strlen(username2));
                strcpy(sender,username2);
            }
            tok = strtok(NULL,"::");
            printf("%s: %s\n",sender,tok);
        }
}
int main(int argc, char* argv[]) {
    printTitle();

    // descrittore socket 
    int socket_desc;
    struct sockaddr_in server_addr = {0}; 

    connection(&socket_desc,&server_addr);

    char op[MAX_OPERATION_LEN];
    User user;

    while(1){
        if(user.username != NULL && user.logged){
            printf("Sono disponibili le seguenti operazioni:\n");
            //printf("\t1 - Aggiorna Chat\n");
            printf("\t1 - Visualizza Messaggi\n");
            printf("\t2 - Logout\n");
            printf("\t%s per terminare\n",SERVER_COMMAND);
            printf("Inserire un operazione: ");
            reader(op,MAX_OPERATION_LEN,"operazione");
            if(!strcmp(SERVER_COMMAND,op)) disconnection(socket_desc);

            else if(!strcmp("1",op)){
                printTitle();
                show_messages(socket_desc,server_addr,&user);
            }
            else if(!strcmp("2",op)){
                printTitle();
                logout(socket_desc,server_addr,&user);
            }
            else{
                printTitle();
            }

        }
        else{
            printf("Benvenuto, sono disponibili le seguenti operazioni:\n");
            printf("\t1 - Login\n");
            printf("\t2 - Sign-in\n");
            printf("\t%s per terminare\n",SERVER_COMMAND);
            printf("Inserire un operazione: ");
            reader(op,MAX_OPERATION_LEN,"operazione");
            if(!strcmp(SERVER_COMMAND,op)) disconnection(socket_desc);
            else if(!strcmp("1",op)) {
                printTitle();
                login(socket_desc, server_addr,&user);
            }
            else if(!strcmp("2",op)){
                printTitle();
                signin(socket_desc,server_addr);
            }
            else{
                printTitle();
            }
        }

    }
}


