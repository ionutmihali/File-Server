FISIERE ARHIVA
    server.c -implementarea server-ului
    client.c -implementarea clientului
    server_helper.h -definiarea unei structuri de date ajutatoare
    README.md -detalii despre implementare
    log.txt -existent sau creat in timpul rularii, mentine istoricul comenzilor primite de server
    Makefile 

CONEXIUNE SI COMUNICARE
    Conexiunea client-server este implementata prin intermediul socket-urilor, comunicarea realizandu-se pe portul 8080.
    Protocolul de comunicare respecta detaliile mentionate in cerinta(IN/OUT), separarea campurilor se realizeaza prin
intermediul caracterului '~', fisierele din lista fiind separate prin '\0'.
    Fiecare client comunica prin intermediul unui thread cu server-ul, durata de viata a unui thread/a unei conexiuni fiind
egala cu timpul necesar serverului pentru indeplinirea unui singur task/unei operatii.


STRUCTURI DE DATE
    Pe server, lista de fisiere este stocata sub forma unei structuri, definita in header-ul server_helper.h:

    struct file
    {
        int dim_nume; -dimensiunea numelui fisierului
        char* nume; -numele fisierului
        int dim; -dimensiunea fisierului
        char** cuv_frec; -cele mai frecvente cuvinte din fisier
        int read; -numarul de operatii de GET realizate pe un fisier, la un moment dat
        int write; -numarul de operatii de UPDATE realizate pe un fisier, la un moment dat
    };


FUNCTIONALITATI
    LIST
    GET 
    PUT *ca argumente, aceasta comanda primeste doar: nr. octeți nume fișier si numele fișierului
    DELETE 
    UPDATE
    SEARCH
    

FUNCTII SI IMPLEMENTARE
    server.c:
        -main() -realizarea conexiunii cu clientii si thread-ului corespunzator fiecarui client
        -handle_client(void *client_fd_ptr) -gestionarea clientilor, primirea buffer-ului in care avem comanda
        -parse_operation(int client_fd, char *buf) -decapsularea mesajului primit si apelarea functiei corespunzatoare comenzii:
            -_list(int client_fd) 
            -_get(char *p, int client_fd) 
            -_put(char *b, int client_fd) 
            -_delete(char *p, int client_fd) 
            -_update(char *p, int client_fd)
            -_search(char *p, int client_fd)
        -_send(int s, char *b) -trimiterea raspunsului catre client
        -_find(char* c, char** buffer) -cauta in lista de fisiere cuvantul dorit si retine in buffer numele fisierelor unde
        l-a gasit

    client.c:
        -main() -realizarea conexiunii cu server-ul, citirea de la standard input a comenzii date de utilizator 
        -parse_operation() -incapsuleaza comanda primita de la client si o trimite catre server
        -parse_receive() -decapsuleaza mesajul de la server si in afiseaza la standard output

    server_helper.h:
        -_log() -creaza si scrie in fisierul "log.txt", toate operatiile primite de server

    sincronizare thread-uri: 
        -daca un thread doreste sa citeasca, in timp ce un altul scrie in acelasi fisier, acesta va
    astepta finalizarea primei operatii si apoi va putea efectua propria operatie. Mod implementare: mutex si variabila conditie, am adaugat o intarziere la comenzile GET si UPDATE pentru a fi vizibila implementarea.

    handler semnale: 
        -am folosit strctura sigaction, functia sigaction si functia signal_handler(int signum), daca este dat unul din
    semnalele SIGINT sau SIGTERM, nu se ma ipermit conexiuni noi, se executa comanda curenta si apoi se inchide server-ul.


MOD DE FUNCTIONARE
    Dupa realizarea conexiunii(rulare Makefile, server si apoi client), clientul introduce una dintre comenzi, iar server-ul 
ii va raspunde cu unul din mesajele: Success, caz in care a reusit sa execute comanda sau Fail, in caz contrar, alaturi de
output-ul comenzii respective. Clientul este deconectat, iar server-ul isi continua rularea, asteptand noi conexiuni.
    Initial, pe server nu se regaseste niciun fisier, acestea trebuie incarcate de catre clienti(comanda PUT).