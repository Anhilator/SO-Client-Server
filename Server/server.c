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

#include "linked_list.h"
#include "server.h"
#include "common.h"
#include "private_chat_struct.h"


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
    fileRead_Database(&database);//vado a leggermi gli utenti dal disco e ricreo il database
}

//Leggo dal file locale tutti gli utenti e li inserisco nella struttura dati database
int fileRead_Database(Database* database){
    FILE * users;
    long fl_size;
    char * buffer;
    size_t res;
    int num;

    char username[50]; 
    char password[50];

    users = fopen ( "user.txt" , "r+" );
    if (users==NULL) {
        handle_error("Errore apertura del file \n");
    }

    fseek (users , 0 , SEEK_END);
    fl_size = ftell (users);
    rewind (users);

    buffer = (char*) malloc (sizeof(char)*fl_size);
    
    res = fread (buffer,1,fl_size,users);
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
        num++;
    }

    //Manca la lettura delle chat per ogni utente

    FILE* chats = fopen("chat.txt", "r");
    if(chats==NULL){// se file_chat non esiste, è inutile leggere le chat 
        if(DEBUG) printf("chat.txt non disponibile\n");
        if(fclose(chats)==EOF)  
            handle_error("fclose di user.txt");
        printf("Lettura da disco completata\n");
        return num; 
    }

    //mi guardo un utente per volta e recupero le sue chat
    User* user=(User*)database->users.first;
    User* aux=user;
    while(aux!=NULL){
        user=aux;
        aux=(User*)user->list.next;
        readFile_Chats(user, chats); //mi leggo le chat di questo utente
    }

    if(fclose(users)==EOF) 
        handle_error("fclose di user.txt");
    if(fclose(chats)==EOF)
        handle_error("fclose di chats.txt");
    free (buffer);
    return num;
}

int readFile_Chats(User* user, FILE* chats){
    

    int num_chats=0;
    if(fscanf(chats, "%d\n", &num_chats)==EOF)  //leggo quante chat ha un dato utente nel file
        handle_error("chats non ha la forma desiderata");
    char buffer[40+40+4+1+1+1]; // username + username + 4*:: + num_messaggi + \n + \0
    char sender[40];
    char receiver[40];
    int n_message;
    for(int i=0; i<num_chats; i++){
        if(fgets(buffer, 40, chats )==NULL){ // leggo i nomi degli utenti della chat
            handle_error("problemi nella fgets");
        }
        strtok(buffer, "\n");
        if(1) 
            printf("stringa letta: %s\n", buffer);
        
        //uso i delimitatori per estrarre sender, receiver e n_message
        
        char* tok=strtok(buffer, "::");// ottengo il primo nome
        assert(tok && "stringhe nel file di lettura non corrette");
        if(sprintf(sender, "%s", tok)<0) //scrivo il primo nome in sender
            handle_error("errore nella sprintf");
        
        tok=strtok(NULL, "::");// ottengo il secondo nome
        assert(tok && "stringhe nel file di lettura non corrette");
        if(sprintf(receiver, "%s", tok)<0)// scrivo il secondo nome in receiver
            handle_error("errore nella sprintf");
        
        tok=strtok(NULL, "::");
        assert(tok && "stringhe nel file di lettura non corrette");
        n_message = atoi(tok);

        User* symmetric_user =User_findByUsername(&database.users, receiver); //mi accerto che il receiver sia presente nel database
                                                           
        assert(symmetric_user && "other_user dovrebbe essere stato già inserito nel database");
        
        //verifico che il sender coincide con l'user passato come parametro
        if(strcmp(sender, user->username)){
            handle_error("sender e nome utente di user non coincidono");
        }

        //creo un oggetto chat con i dati ricavati dal file chat
        // inizializzo una chat tra sender e receiver, la funzione mi tornerà le due chat in entrambi i versi
        ChatListItem** chat= initChat(user, symmetric_user); 
                                                                
        ChatListItem* chat_sender;
        if(chat==NULL){// se chat è null, allora la chat già esisteva (significa che abbiamo letto la chat per l'altro utente)
            // allora la otteniamo con una find, dati gli utenti
            chat_sender=ChatListItem_findByUsers(user, symmetric_user);
        }
        else{
            chat_sender =chat[0]; //mi prendo la chat del sender da chat
            free(chat);
        }
        
        chat_sender->num_messages=n_message;

        if(DEBUG) printf("\nsto per fare readMessage sulla chat %s-%s %d\n", sender, receiver, n_message);
        
        for(int i=0; i< chat_sender->num_messages; i++){ // itero sui messaggi
            Database_readOneMessage(chat_sender, chats);
        }
    }
    return num_chats;    
}


int Database_readOneMessage(ChatListItem* chat, FILE* file_chat){
    assert(chat && "chat in input è nulla");
    assert(file_chat && "file_chat in input è nullo");
    char buf[1024];
    char* sent;

    MessageListItem* message_list_item=(MessageListItem*)calloc(1, sizeof(MessageListItem));
    Message* message=(Message*)calloc(1, sizeof(Message));
    List_insert(&chat->messages, chat->messages.last, (ListItem*) message_list_item);
        
    
    message_list_item->message=message;

    if(fgets(buf,1024, file_chat)==NULL){//leggiamo il carattere che indica se il messaggio è stato inviato o ricevuto
        handle_error("problemi nella prima fscanf in readMeesage");
    }
    char* tok=strtok(buf, "::");
    assert(tok && "stringhe nel file di lettura non corrette");
    sent = tok;
    message_list_item->sent=*sent;

    //printf("sent vale %s\n", sent);
        
    tok=strtok(NULL, "::");

    //rimuovo \n da tok
    strtok(tok, "\n");
    
    assert(tok && "stringhe nel file di lettura non corrette");
    if(sprintf(message->message, "%s", tok)<0)// scrivo il messaggio
        handle_error("errore nella sprintf");

    
    
    if(DEBUG) printf("sent vale %s\n", sent);


    message->len_message = strlen(message->message);
    
    if(DEBUG) printf("lettura fatta\n");
    //printf("message: %s, message_len: %d\n", message->message, message->len_message);
    return 0;
}


//  OPERAZIONI SULLE STRUTTURE DATI //

//Inizializzo un utente con un certo username e una certa password
User* initUser(char username[], char password[]){
    assert(username); // controllo che username e password non siano null
    assert(password);
    int username_len=strlen(username);
    int password_len=strlen(password);
    // controlliamo se l'utente con quel nome già esiste
    if(User_findByUsername(&database.users, username)!=NULL){ // ritorna NULL se non lo trova
        if(DEBUG) printf("utente %s già esiste\n", username);
        return NULL;
    }
    //controllo sul rispetto delle size di password e username
   /* assert(password_len<PASSWORD_SIZE && "password too long");
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

// Inizializzo una chat fornendo in ingresso un sender e un receiver
ChatListItem** initChat(User* sender, User* receiver){
    ListItem* data_user;
    
    if(DEBUG) 
        printf("certo sender nel database\n");
        
    //mi accerto che il sender sia nel database
    data_user = List_find(&(database.users),(ListItem*)sender); 
    assert(data_user && "user1 not in database");

    if(DEBUG) 
        printf("certo utente 2 nel database\n");

    // mi accerto che il receiver sia nel database
    data_user = List_find(&(database.users),(ListItem*)receiver); 
    assert(data_user && "user2 not in database");

    if(ChatListItem_findByUsers(sender, receiver)!=NULL){// se entro nell'if, la chat già esiste
        if(DEBUG) printf("chat tra %s e %s già esiste\n", sender->username, receiver->username);
        return NULL;
    }

    ChatListItem* chat_sender=(ChatListItem*)calloc(1,sizeof(ChatListItem));
    ChatListItem* chat_receiver=(ChatListItem*)calloc(1,sizeof(ChatListItem));

    // inizializzo il sender
    chat_sender->list.prev=0;
    chat_sender->list.next=0;
    List_init(&(chat_sender->messages));// inizializzo la lista dei messaggi 
    chat_sender->sender= sender; //imposto il sender della chat
    chat_sender->receiver=receiver; //imposti il receiver della chat
    chat_sender->num_messages=0; 
    sender->num_chats++; //incremento numero delle chat dell'utente
    chat_sender->other_chat=chat_receiver;
    

    //inizializzo il receiver
    chat_receiver->list.prev=0;
    chat_receiver->list.next=0;
    List_init(&(chat_receiver->messages));
    chat_receiver->sender=receiver;
    chat_receiver->receiver=sender;
    chat_receiver->num_messages=0;
    receiver->num_chats++;
    chat_receiver->other_chat=chat_sender;

    // inserisco le chat nel database
    List_insert(&(sender->chats),sender->chats.last, (ListItem*)chat_sender);
    List_insert(&(receiver->chats),receiver->chats.last, (ListItem*)chat_receiver);
    
    // chat contenente le due chat simmetriche da ritornare
    ChatListItem** chat=(ChatListItem**)calloc(2,sizeof(ChatListItem*));
    chat[0]=chat_sender;
    chat[1]=chat_receiver;
    return chat; // da deallocare
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


ChatListItem* ChatListItem_findByUser(ListHead* head, User* receiver){
    assert(head && "lista vuota");
    assert(receiver && "receiver non presente");


    ChatListItem* chat=(ChatListItem*)head->first;
    ChatListItem* aux=chat;
    
    while(aux!=NULL){
        chat=aux;
        aux=(ChatListItem*)(chat->list.next);
        if(chat->receiver==receiver) return chat;
        
    }
    if(DEBUG) printf("sono uscito dal while\n");
    return NULL;
}

ChatListItem* ChatListItem_findByUsers(User* sender, User* receiver){
    assert(sender && "sender is NULL");
    assert(receiver && "receiver is NULL");
    return ChatListItem_findByUser(&(sender->chats), receiver);

}

// cerco un utente che ha effettuato il login nella lista dei login tramite il sockadrr_in
LoginListItem* LoginListItem_findBySockaddr_in(ListHead* head, struct sockaddr_in client_addr, int sockaddr_len){
    assert(head && "lista vuota");
    assert(head->first && "lista vuota");


    LoginListItem* login=(LoginListItem*)head->first;
    LoginListItem* aux=login;
    
    while(aux!=NULL){
        
        login=aux;
        aux=(LoginListItem*)(login->list.next);

        if(sockaddr_len!=login->sockaddr_len) continue;
        
        if(!memcmp(&(login->client_addr), &client_addr, sockaddr_len)) return login;
    }
    return NULL;
}

// faccio partire il server mettendolo in ascolto sulla porta 
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

// invio una risposta contenuta in bu al client
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

// stampa il titolo 
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

//lg: stampa i dati del singolo utente
void User_print(User* user){
    assert(user && "argomento User_print è NULL");

    printf("Username: %s\n", user->username);
    printf("Password: %s\n", user->password);
    printf("logged: %d\n", user->logged);

    return;
}

void Database_printUser(){
    ListHead users=database.users;
    ListItem* list=users.first;

    while(list){
        User* user=(User*)list;
        User_print(user); //lg: stampa dei dati del singolo utente
        
        printf("\n\n");
        list=list->next;
    }
    return;
}


// scelgo l'operazione da compiere in base alla richiesta del client
void chooseOperation(char buf[], int recv_bytes, struct sockaddr_in client_addr, int sockaddr_len){

    
    char op = buf[0];

    buf += 2;
    recv_bytes -=2;

    if(op == '1'){
        authentication(buf, recv_bytes, client_addr, sockaddr_len);
        Database_printUser();
    } else if(op == '2'){
        signin(buf, recv_bytes, client_addr, sockaddr_len);
    }else if(op == '3'){
        show_chat(client_addr, sockaddr_len);
        
    }else if(op == '4'){
        logout(client_addr, sockaddr_len);
        Database_printUser();
    }else if(op == '5'){
        show_messages(buf, recv_bytes, client_addr, sockaddr_len);
    }
}


// OPERAZIONI SELEZIONABILI DAL CLIENT  //

// utilizzando i dati forniti dal cliente, prova ad effettuare un'autenticazione nel database
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
            
            sprintf(buf, "Login effettuato con successo, bentornato %s", username);
            sendRespone(buf, client_addr, sockaddr_len);// mando il messaggio di conferma del login
            return;
        }
    }
    else{ //username e password sbagliati
        
        sendRespone("Login fallito", client_addr, sockaddr_len);// rispondo con l'errore al client
        return;
    }
}

//effettua la registrazione di un nuovo utente
void signin(char buf[], int recv_butes, struct sockaddr_in client_addr, int sockaddr_len){
    FILE * fl;
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
    if(DEBUG && tok==NULL) printf("errori con sprintf\n");
    
    if(tok==NULL){ 
        if(DEBUG) printf("la stringa non ha il formato voluto, tok NULL");
        sendRespone("Stringa malformata", client_addr, sockaddr_len);
        return;
    }
    int password_len=strlen(tok);
    char password[password_len+1];
    if(sprintf(password,"%s", tok)<0)
        handle_error("sprintf error"); 
    if(DEBUG) printf("password: %s\n", password);

    // provo a inizializzare l'utente
    User* user = initUser(username, password);

    // se initUser mi ha tornato NULL allora l'utente già esiste
    if(user == NULL){
        sendRespone("Username già in uso, riprovare con un altro username", client_addr, sockaddr_len);
        return;
    }

    //altrimenti avremo che la registrazione è andata a buon fine
    sendRespone("Nuovo utente creato correttamente", client_addr, sockaddr_len);
    
    //procedo a scrivere le credenziali del nuovo utente su disco    
    fl = fopen ( "user.txt" , "a" );
    if (fl==NULL) {
        handle_error("Errore apertura del file \n");
    }

    fprintf(fl, "%s::%s\n", username, password);
    fclose(fl);
    return;

}

// effettua il logout di un utente che l'ha richiesto
void logout(struct sockaddr_in client_addr, int sockaddr_len){
    // vedo se il sockaddr_in del cliente che cerca di fare logout è attualmente loggato
    LoginListItem* login=LoginListItem_findBySockaddr_in(&database.login, client_addr, sockaddr_len);

    if(login==NULL){// se non l'ho trovato vuol dire che non era ancora loggato

        sendRespone("Utente non ancora loggato, impossibile effetuare logout", client_addr, sockaddr_len);
        return;
    }
    
    login->user->logged=0;// imposto l'utente come non loggato
    
    List_detach(&database.login, (ListItem*)login); // tolgo l'oggetto di tipo login dell'utente che ha fatto logout dalla lista del database

    sendRespone("Logout effettuato con successo", client_addr, sockaddr_len);

    free(login); // dealoco login
}

// restituisce le chat totali di un utente in questo formato 
// numero_chat_utente_richiedente::mittente_0::num_messaggi_di_0::mittente_1::num_messaggi_di_1::####::mittente_n::num_messaggi_di_n
void show_chat(struct sockaddr_in client_addr, int sockaddr_len){
    // faccio un check se il client è loggato
    LoginListItem* login=LoginListItem_findBySockaddr_in(&database.login, client_addr, sockaddr_len);
    if(login==NULL){
     
        sendRespone("User not logged in\n", client_addr, sockaddr_len);
        return;
    }
    
    User* user=login->user;

    char* buf=(char*)calloc((1+user->num_chats)*1024, sizeof(char));

    
    int number_of_char=sprintf(buf, "%d::", user->num_chats);
    if(number_of_char<0)
        handle_error("sprintf error");

    getChat(buf+3, user->chats);
    //printf("buf = %s \n", buf);
    sendRespone(buf, client_addr, sockaddr_len);
    free(buf);
    
}

// funzione ausiliaria di show_chat
// scrive in buf i receiver con il loro numero di messaggi di una certa chat
void getChat(char buf[],ListHead chats){

    int i=0;
    ChatListItem* chat=(ChatListItem*)chats.first;
    ChatListItem* aux=chat;

    if(aux==NULL){//anche se non ho chat, devo mettere il carattere di terminazione
        i++;
        buf[i-1]='\0';
    }
    while(aux!=NULL){
        chat=aux;
        aux=(ChatListItem*)(chat->list.next);
        User* user=chat->receiver;
        int num=chat->num_messages;
        int temp=sprintf(buf+i, "%s::%d::", user->username, num);
        if(temp<0)
            handle_error("sprintf error");

        i+=temp;

    }
    return;
}

//restituisce all'utente che ha l'ha richiesto una stringa con tutti i messaggi
//che ha nella chat con l'utente passato come parametro
void show_messages(char buf[], int recv_bytes, struct sockaddr_in client_addr, int sockaddr_len){

   /* if(recv_bytes<=0){ 
        if(DEBUG) printf("non ho ricevuti altri byte\n");
        sendResponde("Input errato\n", client_addr, sockaddr_len);
        return;
    }*/
    LoginListItem* login=LoginListItem_findBySockaddr_in(&database.login, client_addr, sockaddr_len);
    if(login==NULL){
        sendRespone("Utente non loggato", client_addr, sockaddr_len);
        return;
    }
    User* sender=login->user;

    User* receiver=User_findByUsername(&database.users, buf); //trovo il receiver rischiesto dall'user
    if(receiver==NULL){
        if(DEBUG) printf("Utente richiesto non esiste\n");
        sendRespone("Utente richiesto inesistente",client_addr,sockaddr_len);
        return;
    }
    // trovo la chat che ha come receiver l'utente richiesto e come chat, le chat del sender
    ChatListItem* chat=ChatListItem_findByUser(&sender->chats, receiver);
    if(chat==NULL){
        sendRespone("Chat richiesta non esistente\n",client_addr,sockaddr_len);
        return;
    }

    printf("s: %s, r: %s \n", sender->username, receiver->username);
    //creo il messggio
    char* response =(char*)calloc(1024*(chat->num_messages+1), sizeof(chat));
    int len=0;
    
    
    getMessages(response, chat->messages);

    //sendRespone(response, client_addr, sockaddr_len);


    free(response);
    return;
}

//funzione ausiliaria di show_messages 
//scrive in buf i messaggi di una certa chat nel formato sent::messaggio1\nsent::messaggio2\n....sent::messaggioN\n
void getMessages(char buf[],ListHead messages){
    int i=0;
    
    MessageListItem* message=(MessageListItem*)messages.first;
    MessageListItem* aux=message;
    if(aux==NULL){
        i++;
        buf[i-1]='\0';
    }

    fflush(stdout);
    while(aux!=NULL){
        message=aux;
        aux=(MessageListItem*)(message->list.next);
        i+=sprintf(buf+i, "%c::%s\n", message->sent, message->message->message);
    }
   // printf("buf= %s \n", buf);
    return;
}