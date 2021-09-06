# SO-Client-Server
Per eseguire il server:
  - cd Server/
  - make
  - ./server
  - 
Se si utilizza il server in locale, settare la costante SERVER_ADDRESS presente in Server/common.h con l'indirizzo di localhost (127.0.0.1), altrimenti, se si vuole far comunicare il server su internet bisognerà aprire la porta 2015 sul proprio router e cambiare, in Server/common.h, la costante SERVER_ADDRESS col proprio ip pubblico (che sarà diverso di volta in volta).

Per esguire il client;
  - cd Client/
  - make
  - ./client

Se ci si vuole connettere ad un server in locale, settare la costante SERVER_ADDRESS presente in Client/common.h con l'indirizzo di localhost (127.0.0.1), altrimenti, se si vuole far comunicare il client con un server remoto tramite internet bisognerà cambiare in Client/common.h con l'ip pubblico del server remoto (che sarà diverso di volta in volta).

Quando si effettuano modifiche e si vuole fare commit, assicurarsi prima di eseguire:
  - cd Server/; make clean; cd ..
  - cd Client/; make clean; cd ..

I dati inerenti a chat e utenti sono scritti su disco dal server negli appositi file user.txt e chat.txt.


