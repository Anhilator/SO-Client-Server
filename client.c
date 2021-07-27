#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>  // htons() and inet_addr()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>

#include "common.h"

//Funzione che rilascia le risorse nel caso di chiusura
void disconnect(int socket_desc){
    // chiudo le varie socket
    int ret = close(socket_desc);
    if (ret < 0) handle_error("Cannot close the socket");

    if (DEBUG) fprintf(stderr, "Socket closed...\n");

    if (DEBUG) fprintf(stderr, "Exiting...\n");

    exit(EXIT_SUCCESS);

}

/*
la richiesta dell'operazione all'utente ha creato parecchi problemi, questo perche' 
la scanf lascia in buffer un new line, quindi e' importante consumare questo new line
affinche nel login la fgets non lo prenda come input per l'user.
*/
void askop(char* op){
    scanf("%c",op);
    int c;
    do{
        c = getchar();
    }while(c != EOF && c != '\n');
}
void reader(char* buf, size_t buf_size){
        int info_len;
        if (fgets(buf,buf_size, stdin) != (char*)buf) {
            fprintf(stderr, "Error while reading from stdin, exiting...\n");
            exit(EXIT_FAILURE);
        }
        info_len = strlen(buf);
        buf[--info_len] = '\0';
}
void sender(char* cmd, int cmd_len,int socket_desc,struct sockaddr_in server_addr){
        int bytes_sent=0, ret;
        while ( bytes_sent < cmd_len) {
            ret = sendto(socket_desc, cmd, cmd_len, 0, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in));
            if (ret == -1 && errno == EINTR) continue;
            if (ret == -1) handle_error("Cannot write to the socket");
            bytes_sent = ret;
        }
}
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
void generate_command(char* user, char* password,char* cmd,char op){
  if (op == 1){
      strcpy(cmd, "1::");
      strcat(cmd,user);
      strcat(cmd,"::");
      strcat(cmd,password);
  }
}
void login(int socket_desc, struct sockaddr_in server_addr,char* logged){
    char log_cmd[44], user[20], password[20],server_response[1024];
    int log_len;

    // sequenza di login
    while (1) {
        char* quit_command = SERVER_COMMAND;

        printf("Enter username: ");
        reader(user,sizeof(user));
        if(strcmp(user,quit_command) == 0) disconnect(socket_desc);
        

        printf("Enter password: ");
        reader(password,sizeof(password));
        if(strcmp(password,quit_command) == 0) disconnect(socket_desc);


        generate_command(user,password,log_cmd,1);

        log_len = strlen(log_cmd);
        sender(log_cmd,log_len,socket_desc,server_addr);
        reciever(server_response,sizeof(server_response), socket_desc);
        printf("Server response: %s\n", server_response);
        if(strcmp(server_response, "Login effettuato con successo") == 0) {
            *logged = 1;
            return;
        }
    }
}



void printTitle(){
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

int main(int argc, char* argv[]) {
    system("clear");
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
    char op, logged =  0;
    printf("Benvenuto, sono disponibili le seguenti operazioni:\n");
    printf("\t1 - Login\n");
    printf("Inserire un operazione: ");

    do{
        askop(&op);
        if(op == '1') login(socket_desc, server_addr,&logged);
        else printf("Si prega di loggarsi prima di continuare, inserire un operazione:");
    }
    while(op != '1');
    
}
