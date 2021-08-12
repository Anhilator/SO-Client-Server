#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>  // htons() and inet_addr()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
#include <termios.h>
#include <signal.h>
#include "common.h"
#include "../CSP.h"
#include "client.h"
#include "linked_list.h"


int socket_desc;
struct sockaddr_in server_addr;
User user;
struct termios terminal;

void printTitle(){
    system("clear");
    char *filename = "title_client.txt";
    FILE *fp = fopen(filename, "r");

    if (fp == NULL)
    {
        handle_error("Errore apertura file\n");
    }

    // reading line by line, max 256 bytes
    const unsigned MAX_LENGTH = 256;
    char buffer[MAX_LENGTH];

    while (fgets(buffer, MAX_LENGTH, fp))
        printf("%s", buffer);

    // close the file
    fclose(fp);

}
void connection(){
        // inizializzo la socket di tipo SOCK_DGRAM, ovvero UDP
    socket_desc = socket(AF_INET, SOCK_DGRAM, 0);
    
    if (socket_desc < 0) handle_error("Could not create socket");
    
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);   // setto l'indirizzo
    server_addr.sin_family      = AF_INET;                     // scelgo il protocollo Ipv4 
    server_addr.sin_port        = htons(SERVER_PORT);          // setto il byte order

}
//Funzione che rilascia le risorse nel caso di chiusura
void disconnection(){
    // chiudo le varie socket
    int ret = close(socket_desc);
    if (ret < 0) handle_error("Cannot close the socket");

    if (DEBUG) fprintf(stderr, "Socket closed...\n");

    if (DEBUG) fprintf(stderr, "Exiting...\n");
    
    terminal.c_lflag &= ~ECHOCTL; 
    terminal.c_lflag |= ECHO; 
    tcsetattr(fileno(stdin),0,&terminal);

    exit(EXIT_SUCCESS);

}

/*Funzione necessaria per leggere correttamente gli input utente.*/
void reader(char* buf, size_t buf_size,char* campo){
        int info_len;
        while(1){
            if (fgets(buf,buf_size, stdin) != (char*)buf) {
                fprintf(stderr, "Error while reading from stdin, exiting...\n");
                exit(EXIT_FAILURE);
            }
            if(buf[0]!= '\n'){
                info_len = strlen(buf);
                buf[--info_len] = '\0';
                return;
            }
            else printf("inserire un %s valido per favore: ",campo);
        }
}

/*Variante della reader che nasconde la password.
In questa funzione utiliziamo la struttura dati termios per modificare lo stdin in modo da poter 
disattivare l'echoing.*/
void reader_password(char* buf, size_t buf_size){
        int info_len;
       
        terminal.c_lflag &= ~ECHO;
        tcsetattr(fileno(stdin),0,&terminal);
        char* ret;
        while(1){
            /*In particolare la chiamata di un'interrupt in questo punto, ossia prima di 
            aver riattivato l'echoing porta a comportamenti indesiderati.*/
            ret= fgets(buf,buf_size, stdin);
            if (ret!= (char*)buf) {
                fprintf(stderr, "Error while reading from stdin, exiting...\n");
                exit(EXIT_FAILURE);
            }
            if(*buf != '\n'){
                info_len = strlen(buf);
                buf[--info_len] = '\0';

                terminal.c_lflag |= ECHO;
                tcsetattr(fileno(stdin),0,&terminal);
                return;
            }
            else printf("\nInserire una password valida per favore: ");
        }
}
/*Funzione che manda informazioni al server*/
void sender(char* cmd, int cmd_len){

        int bytes_sent, ret;
        bytes_sent=ret=0;
        while ( bytes_sent < cmd_len) {
            ret = sendto(socket_desc, cmd, cmd_len, 0, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in));
            if (ret == -1 && errno == EINTR) continue;
            if (ret == -1) handle_error("Cannot write to the socket");
            bytes_sent = ret;
        }
}
/*Funzione che legge le informazioni provenienti dal server*/
void reciever(char* server_response,size_t server_response_len){
        int recv_bytes = 0,ret;
        do {
            ret = recvfrom(socket_desc, server_response, server_response_len, 0, NULL, NULL);
            if (ret == -1 && errno == EINTR) continue;
            if (ret == -1) handle_error("Cannot read from the socket");
	        if (ret == 0) break;
	        recv_bytes = ret;
	    } while ( recv_bytes<=0 );
}
/*Funzione che genera comandi ben formati che verranno poi mandati al server per l'esecuzione*/
void generate_command(char* arg0, char* arg1,char* cmd,char op){
  if (op == 1) sprintf(cmd,"%d::%s::%s",op,arg0,arg1);
  else if(op == 2) sprintf(cmd,"%d::%s::%s",op,arg0,arg1);
  else if(op == 3) sprintf(cmd,"%d",op);
  else if(op == 4) sprintf(cmd,"%d",op);
  else if(op == 5) sprintf(cmd,"%d::%s",op,arg0);
  else if(op == 6) sprintf(cmd,"%d::%s",op,arg0);
  else if(op == 7) sprintf(cmd,"%d::%s::%s",op,arg0,arg1);
}
void destroy_messages(ChatListItem* chat){
    while(chat->messages == NULL){
        ListItem * aux = List_detach(chat->messages,chat->messages->last);
        free(aux);
    } 
}
void destroy_chats(){
    while(user.chats == NULL){
        ListItem* aux = List_detach(user.chats, user.chats->last);
        destroy_messages((ChatListItem*) aux);
        free(aux);
    } 
}
void init_user(char* username, int logged){
    user.logged = logged;
    user.num_chat = 0;
    user.username = (char*) malloc(sizeof(char*) * strlen(username)+1);
    strcpy(user.username, username);
    user.chats = (ListHead*)malloc(sizeof(ListHead)); 
    get_all_chats(user);
}

void destroy_user(){
    destroy_chats();
    free(user.username);
    user.num_chat = 0;
    user.logged = 0;

}

/*funzione che inserisce le chat nell'utente*/
void get_all_chats(){    
    char cmd[10],server_response[100];
    generate_command(NULL, NULL, cmd,3);
    sender(cmd,sizeof(cmd));
    reciever(server_response,sizeof(server_response));
    
    char* tok = strtok(server_response,"::");
    List_init(user.chats);
    user.num_chat = atoi(tok);
    if(user.num_chat == 0) return; 
    char** tmp = malloc(sizeof(char*) * user.num_chat);
    for(int i =0;i < user.num_chat;i++){
        tok = strtok(NULL,"::");
        tmp[i]= malloc(sizeof(char)* strlen(tok));
        strcpy(tmp[i],tok);
    }
    for(int i =0;i < user.num_chat;i++){
        ChatListItem* c_item = (ChatListItem*) malloc(sizeof(ChatListItem));
        ((ListItem*)c_item)->prev =((ListItem*)c_item) ->next  =0;
        char* tok2 = strtok(tmp[i],"**");
        c_item->other_user = malloc(sizeof(char) * (strlen(tok2)+1));
        strcpy(c_item->other_user, tok2);
        tok2 = strtok(NULL,"**");
        c_item->num_messages = atoi(tok2);        
        List_insert(user.chats,(ListItem*)user.chats->last,(ListItem*)c_item);
        }
}
/*funzione ausiliaria che dati due utenti controlla se user ha una chat con user2*/
ChatListItem* find_chat_by_other_user(char* user2){
    ListItem* aux=user.chats->first;
    for(int i =0;i < user.num_chat;i++){
        ChatListItem* tmp = (ChatListItem*) aux;
        if(!strcmp(tmp->other_user,user2)) return (ChatListItem*)List_find(user.chats,(ListItem*)tmp);
        aux = aux->next;
    }
    return NULL;
}
/*funzione che dato un char* rappresentante un altro utente ritorna la chat relativa all'altro utente.*/
ChatListItem* get_messages(char* user2){

    if(user.num_chat <= 0) return NULL;
    char msg_cmd[44],username2[20],server_response[1024];   
    int msg_len;
    show_chat();
    ChatListItem* c_item=(ChatListItem*) malloc(sizeof(ChatListItem));
    if(user2 == NULL){
        printf("Enter username of the user view the chats with: ");
        reader(username2,MAX_USER_LEN,"utente");
    }
    else{
        strcpy(username2,user2);
    }
     
    generate_command(username2,NULL,msg_cmd,5);
    c_item = find_chat_by_other_user(username2);
    if(c_item == NULL){
        return NULL;
    }
    msg_len = strlen(msg_cmd);
    sender(msg_cmd,msg_len);
    reciever(server_response,sizeof(server_response));
    
    get_all_chats();
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
            m_item->message = malloc(sizeof(char*)*(strlen(user.username) + strlen(": ") + strlen(tok)));
            sprintf(m_item->message, "%s: %s",user.username,tok);
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
void login(){
    char log_cmd[44],password[20],username[20],server_response[1024];
    int log_len;
    while (1) {

        printf("Enter username: ");
        reader(username,MAX_USER_LEN,"utente");        

        printf("Enter password: ");
        reader_password(password,MAX_PASSWORD_LEN);

        generate_command(username,password,log_cmd,1);

        log_len = strlen(log_cmd);
        sender(log_cmd,log_len);
        reciever(server_response,sizeof(server_response));

        char success[100];
        sprintf(success,LOGIN_SUCCESS,username); // necessario per una corretta formattazione
   
        if(!strcmp(LOGIN_SUCCESS, server_response) || !strcmp(ALREADY_LOGGED, server_response)) {
            printTitle();
            if(!strcmp(ALREADY_LOGGED, server_response)) printf("%s\n", ALREADY_LOGGED);
            printf("Bentornato nel server, %s.\n", username);
            init_user(username,1);
            get_all_chats();
            return;
        }
        else{
            printTitle();
            printf("É stata inserito un username e/o password sbagliata, riprovare\n");
        }
    }
}
void signin(){
    char sign_cmd[44],server_response[1024],username[20],password[20];
    int sign_len;
    while(1){

        printf("Enter the username you'll be using: ");
        reader(username,MAX_USER_LEN,"utente");        

        printf("Enter password you'll be using: ");
        reader_password(password,MAX_PASSWORD_LEN);


        generate_command(username,password,sign_cmd,2);

        sign_len = strlen(sign_cmd);
        sender(sign_cmd,sign_len);
        reciever(server_response,sizeof(server_response));

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
void logout(){
    char cmd[10];
    generate_command(NULL, NULL, cmd, 4);
    sender(cmd,sizeof(cmd));
    char server_response[100];
    reciever(server_response, sizeof(server_response));
    if(!strcmp(server_response,LOGOUT_SUCCESS)) destroy_user();
    if(DEBUG) printf("%s\n",server_response);

}
void show_chat(){
    
    ListItem* aux = user.chats->first;
    if (aux){
        int pos = 1;
        printf("L'utente ha attualmente %d chat:\n",user.num_chat);

        while(aux){
            ChatListItem* tmp = (ChatListItem*) aux;
            printf("\t%d)Ha %d messaggi con l'utente %s\n",pos++,tmp->num_messages,tmp->other_user);
            aux = aux->next;
        }
    }
    else printf("L'utente non ha attualmente chat.\n");
}
void show_messages(ChatListItem* chat){
    if(chat != NULL){
            ListItem* aux = chat->messages->first;
            if(aux == NULL){
                printf("Al momento non sembrano esserci messaggi, mandane qualcuno per vederlo qui.\n");
                return;
            }
            while(aux){
                MessageListItem* tmp = (MessageListItem*) aux;
                printf("%s\n",tmp->message);
                aux = aux->next;
            }
    }
    else {
                printf("La chat non esiste, creala per vederne i messaggi.\n");
                return;
    }
}
void send_message(char* other_user,char isa_newchat){
    char new_msg_cmd[44],msg[MAX_MESSAGE_SIZE],username2[20],server_response[1024];   
    int msg_len;
    if(other_user == NULL){
        printf("Inserire il nome dell'utente a cui si vuole scrivere: ");
        reader(username2,MAX_USER_LEN,"utente");
    

    }
    else strcpy(username2,other_user);
    if(!isa_newchat) {    
        printf("Inserire il messaggio da mandare: ");
        reader(msg,MAX_USER_LEN,"messaggio");
    }
    
    generate_command(username2,msg,new_msg_cmd,7);
    msg_len = strlen(new_msg_cmd);

    sender(new_msg_cmd,msg_len);
    reciever(server_response,sizeof(server_response));
    printf("%s\n",server_response);

    if(!strcmp(CHAT_DOESNT_EXISTS, server_response)){
        char buf[20];
        while(1){
            printf("La chat non esiste, si vuole creare una chat?\n");
            printf("\t1 - Si\n");
            printf("\t2 - No\n");
            printf("Inserire un operazione: ");
            reader(buf, 20, "operazione");
            

            if(!strcmp(buf,"1")){
                new_chat(username2);
                send_message(username2,1);
                printTitle();
                return;
            }
            else if(!strcmp(buf,"2")){
                printTitle();
                return;
            }
        }
    }
}
void new_chat(char* other_user){
    char new_chat_cmd[44],username2[20],server_response[1024];   
    int msg_len;
    if(other_user == NULL){
        printf("Inserire il nome dell'utente della chat che si vuole creare: ");
        reader(username2,MAX_USER_LEN,"utente");
    }
    else{
        strcpy(username2,other_user);
    }
    
    generate_command(username2,NULL,new_chat_cmd,6);
    msg_len = strlen(new_chat_cmd);

    sender(new_chat_cmd,msg_len);
    reciever(server_response,sizeof(server_response));
    printf("%s\n",server_response);
}
void clientTerminationHandler(int signum){
    printf("\nRichiesta terminazione, arrivederci alla prossima volta.\n");
    if(user.logged == 1)logout();
    disconnection();
}

int main(int argc, char* argv[]) {
    memset(&user,0,sizeof(User));
    memset(&server_addr,0,sizeof(server_addr));
    //disattivo l'echoing dei segnali.

    tcgetattr(fileno(stdin), &terminal);
    terminal.c_lflag |= ECHOCTL;

    printTitle();

    struct sigaction old_action={0};
    struct sigaction new_action={0};
    new_action.sa_handler= clientTerminationHandler;
    sigemptyset (&new_action.sa_mask);
    new_action.sa_flags = 0;

    sigaction(SIGINT, &new_action, &old_action);

    connection();

    char op[MAX_OPERATION_LEN],in_chat = 0, *other_user;
     

    while(1){
        if(user.username != NULL && user.logged && !in_chat){// menu principale
            printf("Sono disponibili le seguenti operazioni:\n");
            printf("\t1 - Vai alla sezione delle chat\n");
            printf("\t2 - Invia un nuovo messaggio\n");
            printf("\t3 - Logout\n");
            printf("\t%s per terminare\n",SERVER_COMMAND);
            printf("Inserire un operazione: ");

            reader(op,MAX_OPERATION_LEN,"operazione");
            if(!strcmp(SERVER_COMMAND,op)) disconnection();

            else if(!strcmp("1",op)){
                printTitle();
                get_all_chats();
                in_chat = 1;
            }
            else if(!strcmp("2",op)){
                printTitle();
                send_message(NULL,0);
            
            }  
            else if(!strcmp("3",op)){
                printTitle();
                logout();
            }
            else if(!strcmp("4",op)){
                printTitle();
                logout();
            }
            else{
                printTitle();
            }
        }
        else if(user.username != NULL && user.logged &&in_chat){// menu delle chat
            get_all_chats();
            show_chat();
            printf("Sono disponibili le seguenti operazioni:\n");
            printf("\t1 - Visualizza messaggi\n");
            printf("\t2 - Invia un nuovo messaggio\n");
            printf("\t3 - Crea una nuova chat\n");
            printf("\t4 - Torna indietro\n");

            printf("\t%s per terminare\n",SERVER_COMMAND);
            printf("Inserire un operazione: ");

            reader(op,MAX_OPERATION_LEN,"operazione");
            if(!strcmp(SERVER_COMMAND,op)) disconnection();

            else if(!strcmp("1",op)){
                printTitle();
                ChatListItem* chat = get_messages(NULL);
                show_messages(chat);
                if(chat != NULL){
                    other_user = malloc(sizeof(char) * (strlen(chat->other_user) + 1) );
                    strcpy(other_user, chat->other_user);
                    while(other_user){
                        printf("Sono disponibili le seguenti operazioni:\n");
                        printf("\t1 - Manda un nuovo messaggio\n");
                        printf("\t2 - Torna alle chat\n");
                        printf("\t3 - Aggiorna chat \n");
                        printf("\t%s per terminare\n",SERVER_COMMAND);
                        printf("Inserire un operazione: ");
                        reader(op,MAX_OPERATION_LEN,"operazione");
                        if(!strcmp(SERVER_COMMAND,op)) disconnection();
                        else if(!strcmp("1",op)){
                            send_message(other_user,0);
                            printTitle();
                            get_all_chats();
                            chat = get_messages(other_user);
                            show_chat();
                            show_messages(chat);
                        }
                        else if(!strcmp("2",op)){
                            printTitle();
                            other_user = NULL;
                        }
                        else if(!strcmp("3",op)){
                            printTitle();
                            get_all_chats();
                            chat = get_messages(other_user);
                            show_chat();
                            show_messages(chat);
                        }
                    }
                    free(other_user);
                    printTitle();                  
                }
            }
            else if(!strcmp("2",op)){
                printTitle();
                send_message(NULL,0);
            }  
            else if(!strcmp("3",op)){
                printTitle();
                new_chat(NULL);
            }
            else if(!strcmp("4",op)){
                printTitle();
                in_chat = 0;
            }

            else{
                printTitle();
            }
        }
        else{//menu di benvenuto
            printf("Benvenuto, sono disponibili le seguenti operazioni:\n");
            printf("\t1 - Login\n");
            printf("\t2 - Sign-in\n");
            printf("\t%s per terminare\n",SERVER_COMMAND);
            printf("Inserire un operazione: ");
            reader(op,MAX_OPERATION_LEN,"operazione");
            if(!strcmp(SERVER_COMMAND,op)) disconnection();
            else if(!strcmp("1",op)) {
                printTitle();
                login();
            }
            else if(!strcmp("2",op)){
                printTitle();
                signin();
            }
            else{
                printTitle();
            }
        }

    }

    sigaction(SIGINT, &old_action, &new_action);


}


