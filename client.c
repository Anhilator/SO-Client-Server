#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>  // htons() and inet_addr()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>

#include "common.h"

void login(int socket_desc, struct sockaddr_in server_addr){
    char buf[1024];
    size_t buf_len = sizeof(buf);
    int msg_len;
    memset(buf,0,buf_len);      

    int ret, bytes_sent,recv_bytes;

    // sequenza di login
    while (1) {
        char* quit_command = SERVER_COMMAND;
        size_t quit_command_len = strlen(quit_command);

        printf("Enter username: ");

        //Leggi una line da stdin
        if (fgets(buf, sizeof(buf), stdin) != (char*)buf) {
            fprintf(stderr, "Error while reading from stdin, exiting...\n");
            exit(EXIT_FAILURE);
        }

        msg_len = strlen(buf);

		// la mando al server
        bytes_sent=0;
        while ( bytes_sent < msg_len) {
            ret = sendto(socket_desc, buf, msg_len, 0, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in));
            if (ret == -1 && errno == EINTR) continue;
            if (ret == -1) handle_error("Cannot write to the socket");
            bytes_sent = ret;
        }

        printf("Enter password: ");

        // idem
        if (fgets(buf, sizeof(buf), stdin) != (char*)buf) {
            fprintf(stderr, "Error while reading from stdin, exiting...\n");
            exit(EXIT_FAILURE);
        }

        msg_len = strlen(buf);

        // idem
        bytes_sent=0;
        while ( bytes_sent < msg_len) {
            ret = sendto(socket_desc, buf, msg_len, 0, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in));
            if (ret == -1 && errno == EINTR) continue;
            if (ret == -1) handle_error("Cannot write to the socket");
            bytes_sent = ret;
        }

        // può tornare utile più avanti
		if (msg_len == quit_command_len && !memcmp(buf, quit_command, quit_command_len)){

            if (DEBUG) fprintf(stderr, "Sent QUIT command ...\n");
             break;
        }

        // ricevo la risposta dal server
	    recv_bytes = 0;
    	do {
            ret = recvfrom(socket_desc, buf, buf_len, 0, NULL, NULL);
            if (ret == -1 && errno == EINTR) continue;
            if (ret == -1) handle_error("Cannot read from the socket");
	        if (ret == 0) break;
	        recv_bytes = ret;

	    } while ( recv_bytes<=0 );

        if (DEBUG) fprintf(stderr, "Received answer of %d bytes...\n",recv_bytes);

        printf("Server response: %s\n", buf); 

        // Se il server mi risponde positivamente esco dal ciclo in quanto mi sono loggato
        if(strcmp(buf, "Login effettuato con successo")==0) return;
    }
}

void disconnect(int socket_desc){
    // chiudo le varie socket
    int ret = close(socket_desc);
    if (ret < 0) handle_error("Cannot close the socket");

    if (DEBUG) fprintf(stderr, "Socket closed...\n");

    if (DEBUG) fprintf(stderr, "Exiting...\n");

    exit(EXIT_SUCCESS);

}

void printTitle(){
    //char ch, username[20] = {}, password[20] = {};

    
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


    

    login(socket_desc, server_addr);

    printf("Loggato");
    while(1);
    
}
