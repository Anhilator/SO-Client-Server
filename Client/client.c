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
    if(user.logged == 1)logout();

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
        bytes_sent=0;
        ret=0;
        while ( bytes_sent < cmd_len) {
            if(cmd != NULL)ret = sendto(socket_desc, cmd,(size_t) cmd_len, 0, (struct sockaddr*) &server_addr, (socklen_t)sizeof(struct sockaddr_in));
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

void destroy_chats(){
    while(user.chats->size >0){
        ChatListItem* tmp = (ChatListItem*)List_detach((&user)->chats, (&user)->chats->first);
        free(tmp->other_user);
        free(tmp);
    } 
}
void init_user(char* username, int logged){
    user.chats = (ListHead*)malloc(sizeof(ListHead));    
   
    user.username = (char*) malloc(sizeof(char*) * (strlen(username)+1));
    strcpy(user.username, username);
    user.logged = logged;
    user.num_chat = 0;
    List_init(user.chats);
    get_all_chats(user);
    
}

void destroy_user(){
    
    
    free(user.username);
    destroy_chats();
    free(user.chats);
    user.num_chat = 0;
    user.logged = 0;
    memset(&user,0,sizeof(User));

}

void chats_update(char* info, ChatListItem* c_item){
    char* tok2 = strtok(info,"**");
    tok2 = strtok(NULL,"**");
    if(tok2!=NULL)c_item->num_messages = atoi(tok2);
}
void chats_add(char* info){
    ChatListItem* c_item = (ChatListItem*) malloc(sizeof(ChatListItem));
    ((ListItem*)c_item)->prev =((ListItem*)c_item) ->next  =0;
    char* tok2 = strtok(info,"**");
    c_item->other_user =(char*) malloc(sizeof(char) * (strlen(tok2)+1));
    strcpy(c_item->other_user, tok2);
    tok2 = strtok(NULL,"**");
    if(tok2!=NULL)c_item->num_messages = atoi(tok2);
    List_insert(user.chats,(ListItem*)user.chats->last,(ListItem*)c_item);
}
/*funzione che inserisce le chat nell'utente*/
void get_all_chats(){    
    char cmd[CMD_LEN],server_response[RESP_LEN];
    memset(cmd,0,CMD_LEN);
    memset(server_response,0, RESP_LEN);
    generate_command(NULL, NULL, cmd,3);
    sender(cmd,CMD_LEN);
    reciever(server_response,RESP_LEN);
    
    char* tok = strtok(server_response,"::");
    
    int old = user.num_chat;
    if(tok!= NULL && user.num_chat <= atoi(tok)){
        user.num_chat = atoi(tok);
        
    }
    else{
        return;
    }
    char** tmp =(char**) malloc(sizeof(char*) * user.num_chat);
    for(int i =0;i < user.num_chat;i++){
        tok = strtok(NULL,"::");
        tmp[i]=(char*) malloc(sizeof(char)* (strlen(tok)+1));
        strcpy(tmp[i],tok);
    }
    ChatListItem* c_item;
     
    if(old == 0 ){
        for(int i =0;i < user.num_chat;i++){
            chats_add(tmp[i]);
            free(tmp[i]);
            }
        free(tmp);
    }
    else if(user.num_chat > old){
        ListItem* aux = user.chats->first;
        for(int i =0;i < user.num_chat;i++){
            if(i<old){
            c_item = (ChatListItem*)aux;
            chats_update(tmp[i],c_item);
            free(tmp[i]);
            aux= aux->next;
            }
            else{
                chats_add(tmp[i]);
                free(tmp[i]);
            }
        }
        free(tmp);
    }
    else{
        ListItem* aux = user.chats->first;
        for(int i =0;i < user.num_chat;i++){
            c_item = (ChatListItem*)aux;
            chats_update(tmp[i],c_item);
            free(tmp[i]);
            aux= aux->next;  
        }
        free(tmp);
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
    char msg_cmd[CMD_LEN],username2[USER_LEN],server_response[RESP_LEN];
    memset(msg_cmd,0,CMD_LEN);
    memset(username2,0,USER_LEN);
    memset(server_response,0,RESP_LEN);
    int msg_len;
    if(user2 == NULL){
        printf("Enter username of the user view the chats with: ");
        reader(username2,USER_LEN,"utente");
    }
    else{
        strcpy(username2,user2);
    }
     
    generate_command(username2,NULL,msg_cmd,5);
    ChatListItem* c_item = find_chat_by_other_user(username2);
    if(c_item == NULL){
        return NULL;
    }
    msg_len = strlen(msg_cmd);
    sender(msg_cmd,msg_len);
    reciever(server_response,sizeof(server_response));
    
    get_all_chats();
    char* tok = strtok(server_response, "\n");
    char** tmp = (char**) malloc(sizeof(char*) * c_item->num_messages);        
    for(int i = 0; i < c_item->num_messages;i++){
        tmp[i]= malloc(sizeof(char)* (strlen(tok)+1));
        strcpy(tmp[i],tok);
        tok = strtok(NULL,"\n");
    }

    for(int i = 0; i < c_item->num_messages;i++){
        tok = strtok(tmp[i],"::");

        if(*tok == 's'){
            tok = strtok(NULL,"::");
            printf("%s: %s\n",user.username,tok);
        }
        else {
            tok = strtok(NULL,"::");
            printf("%s: %s\n",username2,tok);
            }
        free(tmp[i]);

        }
    free(tmp);
    return c_item;
}

/*Sequenza di login, logged viene passato come argomento perche' il client potrebbe essere crashato o
interrotto durante una precedente esecuzione, tale variabile permette di "interrogare" il database 
sull'effettivo stato dell'utente.*/
void login(){
    char log_cmd[CMD_LEN],password[PASS_LEN],username[USER_LEN],server_response[RESP_LEN];
    memset(log_cmd,0,CMD_LEN);
    memset(password,0,PASS_LEN);
    memset(username,0,USER_LEN);
    memset(server_response,0,RESP_LEN);
    int log_len;
    while (1) {

        printf("Enter username: ");
        reader(username,USER_LEN,"utente");        

        printf("Enter password: ");
        reader_password(password,PASS_LEN);

        generate_command(username,password,log_cmd,1);

        log_len = strlen(log_cmd);
        sender(log_cmd,log_len);
        reciever(server_response,RESP_LEN);
   
        if(!strcmp(LOGIN_SUCCESS, server_response) || !strcmp(ALREADY_LOGGED, server_response)) {
            printTitle();
            if(!strcmp(ALREADY_LOGGED, server_response)) printf("%s\n", ALREADY_LOGGED);
            printf("Bentornato nel server, %s.\n", username);
            init_user(username,1);
            return;
        }
        else{
            printTitle();
            printf("?? stata inserito un username e/o password sbagliata, riprovare\n");
        }
    }
}
void signin(){
    char sign_cmd[CMD_LEN],server_response[RESP_LEN],username[USER_LEN],password[PASS_LEN];
    memset(sign_cmd,0,CMD_LEN);
    memset(password,0,PASS_LEN);
    memset(username,0,USER_LEN);
    memset(server_response,0,RESP_LEN);    
    int sign_len;
    while(1){

        printf("Enter the username you'll be using: ");
        reader(username,USER_LEN,"utente");        

        printf("Enter password you'll be using: ");
        reader_password(password,PASS_LEN);


        generate_command(username,password,sign_cmd,2);

        sign_len = strlen(sign_cmd);
        sender(sign_cmd,sign_len);
        reciever(server_response,RESP_LEN);

        if(!strcmp(SIGNIN_SUCCESS, server_response)) {
            printTitle();
            printf("Benvenuto/a nel server, %s.\n",username);
            return;
        }
        else{
            printTitle();
            printf("%s ?? gi?? presente nel server\n",username);
            return;
        }
    }
}
void logout(){
    char cmd[CMD_LEN], server_response[RESP_LEN];
    memset(cmd,0,CMD_LEN);
    memset(server_response,0,RESP_LEN);
    generate_command(NULL, NULL, cmd, 4);
    sender(cmd,sizeof(cmd));
     
    reciever(server_response,RESP_LEN);
    if(!strcmp(server_response,LOGOUT_SUCCESS)) destroy_user();

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

void send_message(char* other_user,char isa_newchat){
    char new_msg_cmd[CMD_LEN],msg[MAX_MSG_SIZE],username2[USER_LEN],server_response[RESP_LEN];  
    memset(new_msg_cmd,0,CMD_LEN);
    memset(msg,0,MAX_MSG_SIZE);
    memset(username2,0,USER_LEN);
    memset(server_response,0,RESP_LEN); 
    int msg_len;
    if(other_user == NULL){
        printf("Inserire il nome dell'utente a cui si vuole scrivere: ");
        reader(username2,USER_LEN,"utente");
    }
    else strcpy(username2,other_user);
    if(!isa_newchat) {    
        printf("Inserire il messaggio da mandare: ");
        reader(msg,MAX_MSG_SIZE,"messaggio");
    }
    if(!strcmp(username2, user.username)) {
        printf("Non si puo' mandare un messaggio a se stessi\n");
        return;
    }
    generate_command(username2,msg,new_msg_cmd,7);
    msg_len = strlen(new_msg_cmd);
    sender(new_msg_cmd,msg_len);
    reciever(server_response,RESP_LEN);
    printf("%s\n",server_response);

    if(!strcmp(CHAT_DOESNT_EXISTS, server_response)){
        char op[OP_LEN];
        while(1){
            printf("La chat non esiste, si vuole creare una chat?\n");
            printf("\t1 - Si\n");
            printf("\t2 - No\n");
            printf("Inserire un operazione: ");
            reader(op, OP_LEN, "operazione");
            

            if(!strcmp(op,"1")){
                new_chat(username2);
                generate_command(username2,msg,new_msg_cmd,7);
                msg_len = strlen(new_msg_cmd);
                sender(new_msg_cmd,msg_len);
                reciever(server_response,RESP_LEN);               
                printTitle();
                printf("Chat creata con successo e messaggio inviato correttamente\n");
                return;
            }
            else if(!strcmp(op,"2")){
                printTitle();
                return;
            }
        }
    }
}
void new_chat(char* other_user){
    char new_chat_cmd[CMD_LEN],username2[USER_LEN],server_response[RESP_LEN];   
    new_chat_cmd[0] = username2[0] = server_response[0]= 0;
    int msg_len;
    if(other_user == NULL){
        printf("Inserire il nome dell'utente con cui si vuole creare la chat: ");
        reader(username2,USER_LEN,"utente");
    }
    else{
        strcpy(username2,other_user);
    }

    if(*username2 == 0 && *user.username == 0 && !strcmp(username2, user.username)) {
        printf("Non si puo' creare una chat con se stessi\n");
        return;
    }
    
    generate_command(username2,NULL,new_chat_cmd,6);
    msg_len = strlen(new_chat_cmd);

    sender(new_chat_cmd,msg_len);
    reciever(server_response,RESP_LEN);
    printf("il server risponde %s\n",server_response);
}
void clientTerminationHandler(int signum){
    printf("\nRichiesta terminazione, arrivederci alla prossima volta.\n");
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

    char op[OP_LEN],in_chat = 0;
     

    while(1){
        if(user.username != NULL && user.logged && !in_chat){// menu principale
            printf("Sono disponibili le seguenti operazioni:\n");
            printf("\t1 - Vai alla sezione delle chat\n");
            printf("\t2 - Invia un nuovo messaggio\n");
            printf("\t3 - Logout\n");
            printf("\t%s per terminare\n",SERVER_COMMAND);
            printf("Inserire un operazione: ");

            reader(op,OP_LEN,"operazione");
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
            printf("\t4 - Aggiorna Chat\n");
            printf("\t5 - Torna indietro\n");

            printf("\t%s per terminare\n",SERVER_COMMAND);
            printf("Inserire un operazione: ");

            reader(op,OP_LEN,"operazione");
            if(!strcmp(SERVER_COMMAND,op)) disconnection();

            else if(!strcmp("1",op)){
                printTitle();
                ChatListItem* chat = get_messages(NULL);
                if(chat != NULL){
                    char in_messages= 1;
                    while(in_messages){
                        printf("Sono disponibili le seguenti operazioni:\n");
                        printf("\t1 - Manda un nuovo messaggio\n");
                        printf("\t2 - Torna alle chat\n");
                        printf("\t3 - Aggiorna chat \n");
                        printf("\t%s per terminare\n",SERVER_COMMAND);
                        printf("Inserire un operazione: ");
                        reader(op,OP_LEN,"operazione");
                        if(!strcmp(SERVER_COMMAND,op)) disconnection();
                        else if(!strcmp("1",op)){
                            send_message(chat->other_user,0);
                            printTitle();
                            get_all_chats();
                            show_chat();
                            chat = get_messages(chat->other_user); 
                        }
                        else if(!strcmp("2",op)){
                            printTitle();
                            in_messages = 0;
                        }
                        else if(!strcmp("3",op)){
                            printTitle();
                            get_all_chats();
                            chat = get_messages(chat->other_user);
                        }
                    }
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
                get_all_chats();
                show_chat();
            }
            else if(!strcmp("5",op)){
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
            reader(op,OP_LEN,"operazione");
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


