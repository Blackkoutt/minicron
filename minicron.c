/*
ps aux | grep nazwa_demona
kill -QUIT PID -wyczysc plik do zapisu
kill -USR1 PID -wczytaj ponownie zadania do wykonania
kill -USR2 PID -zapisz pozostale zadania do wykonania do logu
kill -INT PID -zakoncz demona

less /var/log/syslog|grep Mini-cron
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdbool.h>
#include <syslog.h>
#include "command.h"
#include "list.h"
#include "file.h"
#include "validation.h"

// DEKLARACJE FUNKCJI OBSLUGI SYGNAŁÓW
void handler_end(int signum);
void handler_reload(int signum);
void handler_save(int signum);
void handler_clear(int signum);

volatile bool flag_end = true;  // Flaga ustawiana przez sygnał SIGINT - decyduje o zakończeniu programu
volatile bool flag_save = false;  // Flaga ustawiana przez sygnał SIGUSR2 - decyduje o zapisie pozostałych zadań do logu  
volatile bool flag_reload = false;  // Flaga ustawiana przez sygnał SIGUSR1 - decyduje o ponownym wczytaniu zadań z pliku
volatile bool flag_clear = false; // Flaga ustawiana przez sygnał SIGQUIT - decyduje o wyczyszczeniu pliku wynikowego
Node *head = NULL; // Wskaźnik na początek listy

int main(int argc, char *argv[]) {
    argsValidate(argc, argv[1], argv[2]);   // Walidacja podanych parametrów programu

    // Przypisanie argumentów do zmiennych char*, by wygodniej było z nich korzystać
    char* taskfile = strdup(argv[1]);
    char* outfile = strdup(argv[2]);
    pid_t pid, sid;	// Identyfikator procesu i sesji

    // ------------------------------------------------
    //               DEMONIZACJA 
    // ------------------------------------------------
    pid = fork();	// Utworzenie nowego procesu
    // Jesli fork sie nie uda to konczymy program
    if (pid < 0){
	    fprintf(stderr,"Nie udało się zainicjowac procesu potomnego: %s",strerror(errno));
        exit(EXIT_FAILURE);
    }
    // Jesli fork sie uda to konczymy proces rodzica
    if (pid > 0){
        exit(EXIT_SUCCESS);
    }
    umask(0);	// Zmiana maski pliku

    sid = setsid();	// Nowa sesja dla procesu potomnego
    // Jeśli nie udało się uruchomić nowej sesji to kończymy program
    if (sid < 0){
        fprintf(stderr,"Nie udało się uruchomić nowej sesji dla procesu potomnego: %s",strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Jeśli nie udało się zmienić katalogu roboczego to kończymy program
    if ((chdir("/")) < 0){
        fprintf(stderr,"Nie udało się zmienić katalogu roboczego: %s",strerror(errno));
        exit(EXIT_FAILURE);
    }
    // Zamknięcie standardowych deksryptorów 
    close(STDOUT_FILENO);
    close(STDIN_FILENO);
    close(STDERR_FILENO);

    //INICJALIZACJA DEMONA
    openlog("Mini-cron", LOG_PID, LOG_USER); // Otworz log systemowy
    Read_From_File(&head, taskfile);   // Czytaj dane z pliku i umieść na liście jednokierunkowej
    mergesort(&head);   // Posortuj zadania wzgledem aktualnego czasu algorytmem Merge Sort

    // DEKLARACJE SYGNAŁOW
    signal(SIGINT, handler_end); 
    signal(SIGUSR1, handler_reload);
    signal(SIGUSR2, handler_save);
    signal(SIGQUIT, handler_clear);

    while (flag_end && head != NULL) // Dopóki nie otrzymano sygnału SIGINT lub lista nie jest pusta
	{
        // Czekaj na wykonanie polecenia
        while(time_difference(head->time,"now") != 0 && flag_end){
            // Sygnał SIGUSR2 - zapis do logu
            if(flag_save){
                Save_To_Log(head);
                flag_save = false;    // Ponowne ustawienie flagi flag_save na false
            }
            // Sygnał SIGUSR1 - ponowne wczytanie zadań na listę
            if(flag_reload){
                delete_list(&head);
                Read_From_File(&head,taskfile);
                mergesort(&head);
                syslog(LOG_INFO,"Ponownie wczytano listę zadań");
                flag_reload = false;  // Ponowne ustawienie flagi flag_reload na false
            }
            // Sygnał SIGQUIT - wyczyszczenie pliku wynikowego
            if(flag_clear){
                clearFile(outfile);
                flag_clear = false;   // Ponowne ustawienie flagi flag_clear na false
            }
            sleep(1);   // Śpij przez 1 sekundę
        }
        
        // Jeśli sygnał SIGINT przyszedł w czasie oczekiwania na zadanie należy przerwać także zewnętrzną pętlę while
		if(!flag_end){ break; } 
        syslog(LOG_INFO, "WYKONYWANE ZADANIE: %s", head->entire_command); // Informacja o rozpoczeciu wykonywania zadania
		pid = fork();	// Inicjowanie procesu potomnego
		// Jesli blad forka to konczymy proces
		if(pid < 0){
            syslog(LOG_ERR,"Nie udało się zainicjowac procesu potomnego: %s",strerror(errno));
			exit(EXIT_FAILURE);
		}
		// Proces potomny
		if(pid == 0){
            executeCommand(outfile); // Wykonaj komendę z listy (pojedynczy element listy)
            // Jeśli sygnał SIGINT przyszedł w trakcie wykonywania polecenia zakończ zewnętrzną pętle while po wykonaniu zapisu do pliku
            if(!flag_end) break;
            exit(EXIT_SUCCESS); // Wyjscie z procesu potomnego
		}
        // Proces macierzysty
        if(pid > 0){
            int status;
            waitpid(pid, &status, 0); // Oczekiwanie na zakonczenie procesu potomnego
            if(head->next == NULL) break;
            // W procesie macierzystym nie są widoczne zmiany wierzchołka listy (head) wykonywane przez proces potomny 
            // Przejdz przez wszystkie polecenia w POTOKU (aż do ostatniego) 
            bool nextExist = true; // Czy istnieje następny element listy
            while(head->pipe > 0 && (head->next->pipe == head->pipe)){
                head = head->next;
                if(head->next == NULL){
                    nextExist = false;
                    break;
                }
            }
            if(!nextExist){ break; }
            // Przejdz do kolejnego polecenia
            head = head->next;
        }   
    }
    syslog(LOG_INFO,"Demon zakonczyl prace - brak zadań do wykonania.");
    closelog(); // Zamknij log systemowy
    exit(EXIT_SUCCESS); // Wyjdź z programu
}

void handler_save(int signum){
    flag_save = true; // Po otrzymaniu sygnału SIGUSR2 ustaw flagę flag_save na true
}
void handler_end(int signum){
    flag_end = false; // Po otrzymaniu sygnału SIGINT ustaw flagę flag_end na false
}
void handler_reload(int signum){
    flag_reload = true; // Po otrzymaniu sygnału SIGUSR1 ustaw flagę flag_reload na true
}
void handler_clear(int signum){
    flag_clear = true; // Po otrzymaniu sygnału SIGQUIT ustaw flagę flag_clear na true
}


