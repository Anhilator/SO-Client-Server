#include <errno.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>  // htons()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
#include "server.h"
#include "common.h"
#include "private_chat.h"
#include "linked_list.c"

int socket_desc;
Database database;



int main(int argc, char* argv[]) {

    system("clear"); 
    printTitle();

    setupDatabase();

    startServer();

    exit(EXIT_SUCCESS); 
}

void setupDatabase(){
    database.num_users=0; // setto il numero di utenti a zero
    List_init(&database.users); //inizializzo la lista degli utenti
    List_init(&database.login); //inizializzo la lista dei login
    fileRead_Database(&database);
}

//Leggo dal file locale tutti gli utenti e li inserisco nella struttura dati database
int fileRead_Database(Database* database){
    FILE * fl;
    long fl_size;
    char * buffer;
    size_t res;
        
    char username[50]; 
    char password[50];

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
    int i=0;
    while (strtok_res != NULL)
    {
        if (DEBUG) printf ("%s \n", strtok_res);
       
        if(i==0) {
            sprintf(username, "%s", strtok_res);
            i++;
        }else{
            sprintf(password, "%s", strtok_res);
            User* user = initUser(username, password);
            i--;
        }
        
        strtok_res = strtok (NULL, "::\n");
    }

    //Manca la lettura delle chat per ogni utente

    fclose (fl);
    free (buffer);
}



//Inizializzo un utente con un certo username e una certa password
User* initUser(char username[], char password[]){
    assert(username); // controllo che username e password non siano null
    assert(password);
    int username_len=strlen(username);
    int password_len=strlen(password);
    // controlliamo se l'utente con quel nome già esiste
    /*if(User_findByUsername(&database.users, username)!=NULL){ // ritorna NULL se non lo trova
        if(DEBUG) printf("utente %s già esiste\n", username);
        return NULL;
    }
    //controllo sul rispetto delle size di password e username
    assert(password_len<PASSWORD_SIZE && "password too long");
    assert(username_len<USERNAME_SIZE && "username too long");
    assert(username_len>0 && "username without char");
    assert(password_len>0 && "password without char");*/

    //creo l'utente
    User* user=(User*)calloc(1,sizeof(User));
    user->list.prev=0;
    user->list.next=0;
    
    // scrivo username e password in User
    if(sprintf(user->username, "%s", username)<0)
        handle_error("errore nella sprintf");
    if(sprintf(user->password, "%s", password)<0)
        handle_error("errore nella sprintf");
    
    //inizializza la lista delle chat
    List_init(&(user->chats));
    user->num_chats=0; //utente viene creato senza chat

    //utente loggato
    user->logged=0;
    
    //aggiunta dell'utente al database
    List_insert(&(database.users), database.users.last, (ListItem*) user);
    database.num_users++;
    if(DEBUG){
        printf("Utente creato\n");
        //Database_printUser(user);
    }


    
    return user;
}

// cerco un utente nella lista di un database e lo restituisco se lo trovo
User* User_findByUsername(ListHead* head, const char* username){
    assert(head && "lista vuota");
    assert(username && "username non presente");
    User* user=(User*)head->first;
    User* aux=user;
    
    while(aux!=NULL){
        user=aux;
        aux=(User*)(user->list.next);
        int username_len=strlen(user->username);
        if(DEBUG){
            printf("%s %d e %s %d\n", username, (int)strlen(username), user->username, username_len);
        }
        if(username_len!=strlen(username)) continue;

        if(!memcmp(user->username, username, username_len)) return user;
        
    }
    fflush(stdout);
    return NULL;
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

// aggiorno la struttura dati del database inserendo un utente che risulta loggato e quindi online
void addNewLogin(User* user,struct sockaddr_in client_addr, int sockaddr_len){
    
    LoginListItem* login=(LoginListItem*)calloc(1,sizeof(LoginListItem));
    login->list.next=0;
    login->list.prev=0;
    login->user=user;
    login->client_addr=client_addr;
    login->sockaddr_len=sockaddr_len;

    login->user->logged=1;// setto user loggato
    //if(DEBUG) printf("login effettuato\n");
    //(if(DEBUG) User_print(login->user);
    // inserisco il nuovo utente loggato nella lista di utenti online del database
    List_insert(&(database.login), database.login.last, (ListItem*)login);
}

void sendRespone(char buf[], struct sockaddr_in client_addr, int sockaddr_len){
    int bytes_left = strlen(buf)+1;
    int bytes_sent=0;
    int ret=0;
    while(ret<=0){ 
        ret=sendto(socket_desc, buf, bytes_left, 0, (struct sockaddr*)&client_addr, sockaddr_len);
        if(ret==-1 && errno==EINTR) continue;
        if(ret==-1) handle_error("ERRORE NELLA SENDTO"); 
        bytes_sent=ret; 
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
    
    if(tok==NULL){ 
        if(DEBUG) printf("la stringa non ha il formato voluto, tok NULL");
        //Response_String(ACTION_INPUT_ERROR, client_addr, sockaddr_len);
        return;
    }
    int password_len=strlen(tok);
    char password[password_len+1];
    if(sprintf(password,"%s", tok)<0)
        handle_error("sprintf error"); 
    if(DEBUG) printf("password: %s\n", password);
    

    //vedo se c'è l'utente con quel username nel database
    User* user=User_findByUsername(&database.users, username);
    
    
    //controllo se l'utente è presente, se le lunghezze delle password coincidono e se le password stesse coincidono
    if(user && strlen(user->password)==password_len && !memcmp(user->password, password, password_len)){
        //controllo prima se è già loggato
        if(user->logged){
            
            if(DEBUG) printf("utente già loggato\n");
            sendRespone("Utente già loggato \n", client_addr, sockaddr_len);
            return;
        }
        else{ // se non ero loggato aggiorno il database e notifico il client
            
            if (DEBUG) printf("faccio operazioni di log\n");
            addNewLogin(user, client_addr, sockaddr_len);// aggiungo l'utente alla lista di utenti online
            
            sendRespone("Login effettuato con successo", client_addr, sockaddr_len);// mando il messaggio di conferma del login
            return;
        }
    }
    else{ //username e password sbagliati
        
        sendRespone("Login fallito", client_addr, sockaddr_len);// rispondo con l'errore al client
        return;
    }
}

