#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>  // htons()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
int socket_desc;


#include "server.h"
#include "common.h"

int main(int argc, char* argv[]) {

    system("clear"); 
    printTitle();

    startServer();

    exit(EXIT_SUCCESS); 
}


void startServer(){
    int ret, recv_bytes;

    //descrittori per socket e client

    // setto i campi della struct sockaddr_in
    struct sockaddr_in server_addr = {0}; //, client_addr = {0};
    int sockaddr_len = sizeof(struct sockaddr_in); 

    //inizializzo socket UDP 
    socket_desc = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_desc < 0)
        handle_error("Could not create socket");

    if (DEBUG) fprintf(stderr, "Socket created...\n");

    // attivo SO_REUSEADDR per far ripartire velocemente il server da un crash
    int reuseaddr_opt = 1;
    ret = setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_opt, sizeof(reuseaddr_opt));
    if (ret < 0)
        handle_error("Cannot set SO_REUSEADDR option");

    
    server_addr.sin_addr.s_addr = INADDR_ANY;           //setto sin_addr.s_addr con INADDR_ANY per accettare connesioni da qualunque interfaccia
    server_addr.sin_family      = AF_INET;              //setto sin_familyr con AF_INET per utilizzare il protocollo IPv4
    server_addr.sin_port        = htons(SERVER_PORT);   // setto il byte order in modo inverso

    // binding dell'indirizzo alla socket (devo castare server_addr in quanto è richiesta una struct di tipo sockaddr*)
    ret = bind(socket_desc, (struct sockaddr*) &server_addr, sockaddr_len);
    if (ret < 0)
        handle_error("Cannot bind address to socket");

    if (DEBUG) fprintf(stderr, "Binded address to socket...\n");

    char buf[1024]; // buffer in cui andrò a scrivere il messaggio
    size_t buf_len = sizeof(buf);
    //int msg_len; 
    memset(buf,0,buf_len); //setto il buffer tutto a 0

    struct sockaddr_in client_addr;
    sockaddr_len = sizeof(client_addr);
    // attendo connessioni in modo sequenziale
    while (1) {
		
        if (DEBUG) fprintf(stderr, "opening connection handler...\n");
         

        recv_bytes = 0;
        do {
            // ricevo i byte comunicati dal client e li metto nel buffer 
            recv_bytes = recvfrom(socket_desc, buf, buf_len-1, 0,(struct sockaddr*) &client_addr, (socklen_t*) &sockaddr_len);
            if (recv_bytes == -1 && errno == EINTR) continue; 
            if (recv_bytes == -1) handle_error("Cannot read from the socket");
            if (recv_bytes == 0) break;
	    } while ( recv_bytes <= 0 ); // devo evitare la possibilità di messaggi letti parzialmente
        
        buf[recv_bytes] = '\0';
        // formato comando da client numero_operazione::campi per quell'operazione

        chooseOperation(buf, recv_bytes, client_addr, sockaddr_len);
    

    }
}
void sendRespone(char buf[], int bytes_left, struct sockaddr_in client_addr, int sockaddr_len){

    int bytes_sent=0;
    int ret=0;
    while(ret<=0){ 
        ret=sendto(socket_desc, buf, bytes_left, 0, (struct sockaddr*)&client_addr, sockaddr_len);
        if(ret==-1 && errno==EINTR) continue;//lg: errore dovuto all'interruzione, riproviamo
        if(ret==-1) handle_error("ERRORE NELLA SENDTO"); //lg: errore nella sendto
        bytes_sent=ret; //lg: se invio andato bene, settiamo il numero di bytes_sent a ret
    }

}

void chooseOperation(char buf[], int recv_bytes, struct sockaddr_in client_addr, int sockaddr_len){

    char op = buf[0];

    buf += 2;
    recv_bytes -=2;

    if(op == '1'){
        authentication(buf, recv_bytes, client_addr, sockaddr_len);
    }
}


void* connection_handler(int socket_desc) {
    int ret, recv_bytes, bytes_sent;


    char buf[1024]; // buffer in cui andrò a scrivere il messaggio
    size_t buf_len = sizeof(buf);
    //int msg_len; 
    memset(buf,0,buf_len); //setto il buffer tutto a 0

    struct sockaddr_in client_addr;
    int sockaddr_len = sizeof(client_addr); 
    

    // echo loop
    while (1) {
        
        recv_bytes = 0;
        do {
            // ricevo i byte comunicati dal client e li metto nel buffer 
            recv_bytes = recvfrom(socket_desc, buf, buf_len, 0,(struct sockaddr*) &client_addr, (socklen_t*) &sockaddr_len);
            if (recv_bytes == -1 && errno == EINTR) continue; 
            if (recv_bytes == -1) handle_error("Cannot read from the socket");
            if (recv_bytes == 0) break;
		} while ( recv_bytes <= 0 ); // devo evitare la possibilità di messaggi letti parzialmente

        
        if (DEBUG) fprintf(stderr, "Messaggio di %d bytes...\n",recv_bytes);


        // se non è il messaggio di terminazione lo rimando indietro come ack
        bytes_sent=0;
        while ( bytes_sent < recv_bytes) {
            ret = sendto(socket_desc, buf, recv_bytes, 0,(struct sockaddr*) &client_addr, sockaddr_len);
            if (ret == -1 && errno == EINTR) continue;
            if (ret == -1) handle_error("Cannot write to the socket");
            bytes_sent += ret;
        }
    }


    // chiudo la socket
    ret = close(socket_desc);
    if (ret < 0) handle_error("Cannot close socket for incoming connection");

    if (DEBUG) fprintf(stderr, "Socket closed...\n");

    return NULL;
}

void printTitle(){
    
    char *filename = "title_server.txt";
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

int file_authentication(char * username, char * password){
    FILE * fl;
    long fl_size;
    char * buffer;
    size_t res;

    //estraggo il carattere \n da username e password
    strtok(username, "\n"); 
    strtok(password, "\n");
        
    fl = fopen ( "user.txt" , "r+" );
    if (fl==NULL) {
        handle_error("Errore apertura del file \n");
    }

    fseek (fl , 0 , SEEK_END);
    fl_size = ftell (fl);
    rewind (fl);

    buffer = (char*) malloc (sizeof(char)*fl_size);
    

    res = fread (buffer,1,fl_size,fl);
    if (res != fl_size) {
        handle_error("Errore in lettura file");
    }

    char * strtok_res;
    strtok_res = strtok(buffer, "::");
    int i=0, check = 0;
    while (strtok_res != NULL)
    {
        if (DEBUG) printf ("%s \n", strtok_res);
       
        if(i==0 && strcmp(strtok_res, username) == 0) {check++; i++;}
        else {
            if(check == 1 && i==1 && (strcmp(strtok_res, password) == 0)) return 1;
            else {check = 0; i=0;}
        } 
        
        strtok_res = strtok (NULL, "::\n");
    }

    fclose (fl);
    free (buffer);
    return 0;
}

void authentication(char buf[], int recv_bytes, struct sockaddr_in client_addr, int sockaddr_len){

    //time_t rawtime;
    //struct tm * timeinfo;



    char* tok=strtok(buf,"::");// prendo lo username
    if(DEBUG && tok==NULL) printf("problemi con tok\n");
    
    if(tok==NULL){ // se non riesco a prendere il nome allora input errato
        if(DEBUG) printf("la stringa non ha il formato voluto, tok NULL");
        return;
    }
    int username_len=strlen(tok);
    char username[username_len+1]; 
    if(sprintf(username,"%s", tok)<0) // ricavo lo username da buf
        handle_error("sprintf error");
    if(DEBUG) printf("username: %s\n", username);

    
    tok=strtok(NULL,"::");
    if(DEBUG && tok==NULL) printf("PROBLEMI\n");
    
    if(tok==NULL){ //lg: se non riesco a prendere la password allora input errato
        if(DEBUG) printf("la stringa non ha il formato voluto, tok NULL");
        //Response_String(ACTION_INPUT_ERROR, client_addr, sockaddr_len);
        return;
    }
    int password_len=strlen(tok);
    char password[password_len+1];
    if(sprintf(password,"%s", tok)<0)
        handle_error("sprintf error"); 
    if(DEBUG) printf("password: %s\n", password);
    
    if(file_authentication(username, password)) {
        printf("Login successo, procedo ad invio messaggio \n");
        buf = "Login effettuato con successo";
        sendRespone(buf, strlen(buf)+1, client_addr, sockaddr_len);
    }else{
        buf = "Login fallito";
        sendRespone(buf, strlen(buf)+1, client_addr, sockaddr_len);
    }
}

