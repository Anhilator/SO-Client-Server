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
#include "linked_list.h"



void init_user(User* user, char* username, int logged,int socket_desc,struct sockaddr_in server_addr){
    user->username = (char*)malloc(sizeof(char*) * strlen(username)+1);
    strcpy(user->username, username);
    user->chats = (ListHead*)malloc(sizeof(ListHead));
    user->logged = logged;
    get_all_chats(socket_desc,server_addr,user);
    user->num_chat = 0;
    
}

void destroy_user(User* user){
    free(user->chats);
    free(user->username);
    user->num_chat = 0;
    user->logged = 0;
}

/*funzione che inserisce le chat nell'utente*/
void get_all_chats(int socket_desc,struct sockaddr_in server_addr,User* user){    
    char cmd[10],server_response[100];
    generate_command(NULL, NULL, cmd,3);
    sender(cmd,sizeof(cmd),socket_desc,server_addr);
    reciever(server_response,sizeof(server_response),socket_desc);
    
    char* tok = strtok(server_response,"::");
    List_init(user->chats);
    user->num_chat = atoi(tok);
    if(user->num_chat == 0) return; 
    char** tmp = malloc(sizeof(char*) * user->num_chat);
    for(int i =0;i < user->num_chat;i++){
        tok = strtok(NULL,"::");
        tmp[i]= malloc(sizeof(char)* strlen(tok));
        strcpy(tmp[i],tok);
    }
    for(int i =0;i < user->num_chat;i++){
        ChatListItem* c_item = (ChatListItem*) malloc(sizeof(ChatListItem));
        ((ListItem*)c_item)->prev =((ListItem*)c_item) ->next  =0;
        char* tok2 = strtok(tmp[i],"**");
        c_item->other_user = malloc(sizeof(char) * (strlen(tok2)+1));
        strcpy(c_item->other_user, tok2);
        tok2 = strtok(NULL,"**");
        c_item->num_messages = atoi(tok2);        
        List_insert(user->chats,(ListItem*)user->chats->last,(ListItem*)c_item);
        }
}
/*funzione ausiliaria che dati due utenti controlla se user ha una chat con user2*/
ChatListItem* find_chat_by_other_user(User* user, char* user2){
    ListItem* aux=user->chats->first;
    for(int i =0;i < user->num_chat;i++){
        ChatListItem* tmp = (ChatListItem*) aux;
        if(!strcmp(tmp->other_user,user2)) return (ChatListItem*)List_find(user->chats,(ListItem*)tmp);
        aux = aux->next;
    }
    return NULL;
}
/*funzione che dato un char* rappresentante un altro utente ritorna la chat relativa all'altro utente.*/
ChatListItem* get_messages(int socket_desc, struct sockaddr_in server_addr,User* user,char* user2){

    if(user->num_chat <= 0) return NULL;
    char msg_cmd[44],username2[20],server_response[1024];   
    int msg_len;
    ChatListItem* c_item=(ChatListItem*) malloc(sizeof(ChatListItem));
    if(user2 == NULL){
        printf("Enter username of the user view the chats with: ");
        reader(username2,MAX_USER_LEN,"utente");
    }
    else{
        strcpy(username2,user2);
    }
     
    generate_command(username2,NULL,msg_cmd,5);
    c_item = find_chat_by_other_user(user, username2);
    if(c_item == NULL){
        printf("Non si hanno chat con questo utente\n");
        return NULL;
    }
    msg_len = strlen(msg_cmd);
    sender(msg_cmd,msg_len,socket_desc,server_addr);
    reciever(server_response,sizeof(server_response), socket_desc);
    
    get_all_chats(socket_desc, server_addr, user);
    char* tok = strtok(server_response, "\n");
    char** tmp = malloc(sizeof(char*) * c_item->num_messages);        
    for(int i = 0; i < c_item->num_messages;i++){

        tmp[i]= malloc(sizeof(char)* strlen(tok));
        strcpy(tmp[i],tok);
        tok = strtok(NULL,"\n");
    }
    c_item->messages = malloc(sizeof(ListHead));
    List_init(c_item->messages);
    for(int i = 0; i < c_item->num_messages;i++){
        tok = strtok(tmp[i],"::");
        MessageListItem* m_item = (MessageListItem*)malloc(sizeof(MessageListItem));
        ((ListItem*)m_item)->prev =((ListItem*)m_item) ->next  =0;

        if(*tok == 's'){
            tok = strtok(NULL,"::");
            m_item->message = malloc(sizeof(char*)*(strlen(user->username) + strlen(": ") + strlen(tok)));
            sprintf(m_item->message, "%s: %s",user->username,tok);
        }
        else {
            tok = strtok(NULL,"::");
            m_item->message = malloc(sizeof(char*)*(strlen(username2) + strlen(": ") + strlen(tok)));
            sprintf(m_item->message, "%s: %s",username2,tok);
            }
            List_insert(c_item->messages,c_item->messages->last, (ListItem*) m_item);
        }
    
    return c_item;
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
            get_all_chats(socket_desc,server_addr,user);
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
            get_all_chats(socket_desc,server_addr,user);
            /*reciever(server_response,sizeof(server_response), socket_desc);
            printf("%s server response\n",server_response);*/

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
void show_chat(User* user){
    
    ListItem* aux = user->chats->first;
    if (aux){
        int pos = 1;
        printf("L'utente ha attualmente %d chat:\n",user->num_chat);

        while(aux){
            ChatListItem* tmp = (ChatListItem*) aux;
            printf("\t%d)Ha %d messaggi con l'utente %s\n",pos++,tmp->num_messages,tmp->other_user);
            aux = aux->next;
        }
    }
    else printf("L'utente non ha attualmente chat.\n");
}
void show_messages(ChatListItem* chat){
    if(chat == NULL) return;
    ListItem* aux = chat->messages->first;
    while(aux){
          MessageListItem* tmp = (MessageListItem*) aux;
          printf("%s\n",tmp->message);
          aux = aux->next;
    }
}
void send_message(int socket_desc, struct sockaddr_in server_addr, User* user,char* user2){
    char new_msg_cmd[44],msg[50],username2[20],server_response[1024];   
    int msg_len;
    
    if(!DEBUG){
    if(user2 == NULL){
    printf("Inserire il nome dell'utente a cui si vuole scrivere: ");
    reader(username2,MAX_USER_LEN,"utente");
    }
    else strcpy(username2,user2);

    printf("Inserire il messaggio da mandare: ");
    reader(msg,MAX_USER_LEN,"messaggio");
    }
    else{
        strcpy(username2,"Jose");
        strcpy(msg,"coppola");
    }
    generate_command(username2,msg,new_msg_cmd,7);
    msg_len = strlen(new_msg_cmd);

    sender(new_msg_cmd,msg_len,socket_desc,server_addr);
    reciever(server_response,sizeof(server_response), socket_desc);
    printf("%s\n",server_response);
}
int main(int argc, char* argv[]) {
    printTitle();

    // descrittore socket 
    int socket_desc;
    struct sockaddr_in server_addr = {0}; 

    connection(&socket_desc,&server_addr);

    char op[MAX_OPERATION_LEN],in_chat = 0;
    char* other_user;
    User user; 

    while(1){
        if(user.username != NULL && user.logged && !in_chat){

            printf("Sono disponibili le seguenti operazioni:\n");
            printf("\t1 - Vai alla sezione delle chat\n");
            printf("\t2 - Invia un nuovo messaggio\n");
            printf("\t3 - Logout\n");
            printf("\t%s per terminare\n",SERVER_COMMAND);
            printf("Inserire un operazione: ");

            reader(op,MAX_OPERATION_LEN,"operazione");
            if(!strcmp(SERVER_COMMAND,op)) disconnection(socket_desc);

            else if(!strcmp("1",op)){
                printTitle();
                get_all_chats(socket_desc, server_addr, &user);
                in_chat = 1;
            }
            else if(!strcmp("2",op)){
                printTitle();
                send_message(socket_desc, server_addr, &user,NULL);
               
            }  
            else if(!strcmp("3",op)){
                printTitle();
                logout(socket_desc,server_addr, &user);
            }
            else if(!strcmp("4",op)){
                printTitle();
                logout(socket_desc,server_addr,&user);
            }


            else{
                printTitle();
            }
        }
        else if(user.username != NULL && user.logged &&in_chat){
            get_all_chats(socket_desc,server_addr,&user);
            show_chat(&user);
            printf("Sono disponibili le seguenti operazioni:\n");
            printf("\t1 - Visualizza messaggi\n");
            printf("\t2 - Invia un nuovo messaggio\n");
            printf("\t3 - Crea una nuova chat\n");
            printf("\t4 - Torna indietro\n");

            printf("\t%s per terminare\n",SERVER_COMMAND);
            printf("Inserire un operazione: ");

            reader(op,MAX_OPERATION_LEN,"operazione");
            if(!strcmp(SERVER_COMMAND,op)) disconnection(socket_desc);

            else if(!strcmp("1",op)){
                printTitle();
                ChatListItem* chat = get_messages(socket_desc, server_addr, &user,NULL);
                show_messages(chat);
                other_user = malloc(sizeof(char) * (strlen(chat->other_user) + 1) );
                strcpy(other_user, chat->other_user);
                while(other_user){
                    printf("Sono disponibili le seguenti operazioni:\n");
                    printf("\t1 - Manda un nuovo messaggio\n");
                    printf("\t2 - Torna alle chat\n");
                    printf("\t%s per terminare\n",SERVER_COMMAND);
                    printf("Inserire un operazione: ");
                    reader(op,MAX_OPERATION_LEN,"operazione");
                    if(!strcmp(SERVER_COMMAND,op)) disconnection(socket_desc);
                    else if(!strcmp("1",op)){
                        send_message(socket_desc, server_addr,&user, other_user);
                        printTitle();
                        get_all_chats(socket_desc, server_addr, &user);
                        chat = get_messages(socket_desc, server_addr, &user,other_user);
                        show_chat(&user);
                        show_messages(chat);
                    }
                    else if(!strcmp("2",op)){
                        printTitle();
                        other_user = NULL;
                    }  
                }
                free(other_user);
                printTitle();
            }
            else if(!strcmp("2",op)){
                printTitle();
                send_message(socket_desc,server_addr, &user,NULL);
                
               
            }  
            else if(!strcmp("3",op)){
                printTitle();
                printf("Ci stiamo lavorando\n");
            }
            else if(!strcmp("4",op)){
                printTitle();
                in_chat = 0;
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


