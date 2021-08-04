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
void connection(int* socket_desc, struct sockaddr_in* server_addr){
        // inizializzo la socket di tipo SOCK_DGRAM, ovvero UDP
    *socket_desc = socket(AF_INET, SOCK_DGRAM, 0);
    
    if (*socket_desc < 0) handle_error("Could not create socket");
    
    server_addr->sin_addr.s_addr = inet_addr(SERVER_ADDRESS);   // setto l'indirizzo
    server_addr->sin_family      = AF_INET;                     // scelgo il protocollo Ipv4 
    server_addr->sin_port        = htons(SERVER_PORT);          // setto il byte order
;
}
//Funzione che rilascia le risorse nel caso di chiusura
void disconnection(int socket_desc){
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
void generate_command(char* arg0, char* arg1,char* cmd,char op){
  if (op == 1) sprintf(cmd,"%d::%s::%s",op,arg0,arg1);
  else if(op == 2) sprintf(cmd,"%d::%s::%s",op,arg0,arg1);
  else if(op == 3) sprintf(cmd,"%d",op);
  else if(op == 4) sprintf(cmd,"%d",op);
  else if(op == 5) sprintf(cmd,"%d::%s",op,arg0);
}