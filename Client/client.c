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

#define MAX_USER_LEN 20
#define MAX_PASSWORD_LEN 20
#define MAX_OPERATION_LEN 6

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
//Funzione che rilascia le risorse nel caso di chiusura
void disconnect(int socket_desc){
    // chiudo le varie socket
    int ret = close(socket_desc);
    if (ret < 0) handle_error("Cannot close the socket");

    if (DEBUG) fprintf(stderr, "Socket closed...\n");

    if (DEBUG) fprintf(stderr, "Exiting...\n");

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
disattivare l'echoing.
Tale funzione e' in alfa e pertanto e' fortemente sconsigliato interrompere l'esecuzione prima dell'inserimento
questo perche' potrebbe causare problemi indesiderati al Terminale*/
void reader_password(char* buf, size_t buf_size){
        int info_len;
        struct termios terminal;
        tcgetattr(fileno(stdin), &terminal);
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
void sender(char* cmd, int cmd_len,int socket_desc,struct sockaddr_in server_addr){
        int bytes_sent=0, ret;
        while ( bytes_sent < cmd_len) {
            ret = sendto(socket_desc, cmd, cmd_len, 0, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in));
            if (ret == -1 && errno == EINTR) continue;
            if (ret == -1) handle_error("Cannot write to the socket");
            bytes_sent = ret;
        }
}
/*Funzione che legge le informazioni provenienti dal server*/
void reciever(char* server_response,size_t server_response_len, int socket_desc){
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
void generate_command(char* user, char* password,char* cmd,char op){
  if (op == 1) sprintf(cmd,"%d::%s::%s",op,user,password);
  else if(op == 2) sprintf(cmd,"%d::%s::%s",op,user,password);
  else if(op == 4) sprintf(cmd,"%d",op);
}

/*Sequenza di login, logged viene passato come argomento perche' il client potrebbe essere crashato o
interrotto durante una precedente esecuzione, tale variabile permette di "interrogare" il database 
sull'effettivo stato dell'utente.*/
void login(int socket_desc, struct sockaddr_in server_addr,char* logged,char* user, char* password){
    char log_cmd[44],server_response[1024];
    int log_len;
    while (1) {

        printf("Enter username: ");
        reader(user,MAX_USER_LEN,"utente");        

        printf("Enter password: ");
        reader_password(password,MAX_PASSWORD_LEN);


        generate_command(user,password,log_cmd,1);

        log_len = strlen(log_cmd);
        sender(log_cmd,log_len,socket_desc,server_addr);
        reciever(server_response,sizeof(server_response), socket_desc);

        char success[62], already_logged[22];
        sprintf(success,"Login effettuato con successo, bentornato %s",user);
        strcpy(already_logged,"Utente già loggato \n");
        printf("Server responde = %s\n",server_response);

        if(!strcmp(success, server_response) || !strcmp(already_logged, server_response)) {
            printTitle();
            if(!strcmp(already_logged, server_response)) printf("%s\n", already_logged);
            printf("Bentornato nel server, %s.\n", user);
            *logged = 1;
            return;
        }
        else{
            //printTitle();
            printf("É stata inserito un username e/o password sbagliata, riprovare\n");
        }
    }
}

void signin(int socket_desc,struct sockaddr_in server_addr){
    char sign_cmd[44],server_response[1024],user[20],password[20];
    int sign_len;
    while(1){

        printf("Enter the username you'll be using: ");
        reader(user,MAX_USER_LEN,"utente");        

        printf("Enter password you'll be using: ");
        reader_password(password,MAX_PASSWORD_LEN);


        generate_command(user,password,sign_cmd,2);

        sign_len = strlen(sign_cmd);
        sender(sign_cmd,sign_len,socket_desc,server_addr);
        reciever(server_response,sizeof(server_response), socket_desc);

        char success[100], already_signed[100];
        strcpy(success,"Nuovo utente creato correttamente");
        strcpy(already_signed,"Username già in uso, riprovare con un altro username");

        if(!strcmp(success, server_response)) {
            printTitle();
            printf("Bentornato nel server, %s.\n",user);
            return;
        }
        else{
            printTitle();
            printf("%s é già presente nel server\n",user);
            return;
        }
    }
}
void logout(int socket_desc,struct sockaddr_in server_addr,char* logged){
    char cmd[10];
    generate_command(NULL, NULL, cmd, 4);
    sender(cmd,sizeof(cmd),socket_desc,server_addr);
    *logged = 0;
    char server_response[20];
    reciever(server_response, sizeof(server_response), socket_desc);

}
int main(int argc, char* argv[]) {
    printTitle();
    
    
    // descrittore socket 
    int socket_desc;
    struct sockaddr_in server_addr = {0}; 

    // inizializzo la socket di tipo SOCK_DGRAM, ovvero UDP
    socket_desc = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_desc < 0)
        handle_error("Could not create socket");

    if (DEBUG) fprintf(stderr, "Socket created...\n");

    
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);   // setto l'indirizzo
    server_addr.sin_family      = AF_INET;                     // scelgo il protocollo Ipv4 
    server_addr.sin_port        = htons(SERVER_PORT);          // setto il byte order
    char op[MAX_OPERATION_LEN], logged =  0, user[MAX_USER_LEN], password[MAX_PASSWORD_LEN];

    while(1){
        if(!logged){
            printf("Benvenuto, sono disponibili le seguenti operazioni:\n");
            printf("\t1 - Login\n");
            printf("\t2 - Sign-in\n");
            printf("\t%s per terminare\n",SERVER_COMMAND);
            printf("Inserire un operazione: ");
            reader(op,MAX_OPERATION_LEN,"operazione");
            if(!strcmp(SERVER_COMMAND,op)) disconnect(socket_desc);
            else if(!strcmp("1",op)) {
                printTitle();
                login(socket_desc, server_addr,&logged,user,password);
            }
            else if(!strcmp("2",op)){
                printTitle();
                signin(socket_desc,server_addr);
            }
        }
        else if(logged){
            printf("Sono disponibili le seguenti operazioni:\n");
            printf("\t1 - Chat\n");
            printf("\t2 - Logout\n");
            printf("\t%s per terminare\n",SERVER_COMMAND);
            printf("Inserire un operazione: ");
            reader(op,MAX_OPERATION_LEN,"operazione");
            if(!strcmp(SERVER_COMMAND,op)) disconnect(socket_desc);
            else if(!strcmp("1",op)) {
                printTitle();
                printf("Operazione non ancora disponibile\n");
            }
            else if(!strcmp("2",op)){
                printTitle();
                logout(socket_desc,server_addr,&logged);
            }

        }
    }
}
