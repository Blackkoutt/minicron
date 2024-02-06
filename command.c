#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <syslog.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "command.h"
#include "list.h"
#include "file.h"

void executeCommand(char* outfile){
    int status;
    int stdout_pipe[2]; // Potok do przekierowania STDOUT
    int stderr_pipe[2]; // Potok do przekierowania STDERR
    int fd_pipe_in[2]; // Potok do przekazywania wyników między poleceniami (procesami) STDOUT => STDIN
    int pipe_command_number = 0; // Licznik poleceń w potoku
    bool flag_pipe = true; // Flaga określająca czy polecenie znajduje się w potoku (proces macierzysty zmienia ta flage)
    int i = 0;
    char buf[SIZE_BUF_OUT] = {0}; // Bufor do odebrania wyniku z stderr gdy wystąpi błąd podczas przetwarzania potoku poleceń 
    pipe_command_number = 0; // Określa które polecenie z potoku jest przetwarzane

    // PETLA WYKONUJACA POTOKI POLECEN np ls-al|wc -c (w tym też zwykłe polecenia np ls -al)
    while (head != NULL && flag_end && flag_pipe) {
        int args_count = (head->argc)+2; // Liczba argumentów (argumenty polecenia + nazwa funkcji + NULL)
        if(head->argc == 0){args_count = 2;} // Jeśli brak argumentów funkcji to liczba argumentów jest równa 2 (nazwa funkcji + NULL)
        char *args[args_count]; // Tablica argumentów
        args[0] = head->command; // Pierwszy argument to nazwa funkcji
        args[args_count-1] = NULL; // Ostatni argument to NULL
        // Pętla wpisująca pozostałe argumenty polecenia do tablicy args
        for(int i=1; i < args_count-1 && args_count != 2; i++){ 
            args[i]=head->args[i-1]; // Dodatnie argumentów do tablicy
        }

        // INICJOWANIE POTOKÓW stderr_pipe i stdout_pipe (każdy proces posiada odzielny nowy potok)
        if(pipe(stderr_pipe) == -1 || pipe(stdout_pipe) == -1){
            syslog(LOG_ERR, "Bład utworzenia potoków stdout_pipe/stderr_pipe %s",strerror(errno));
            exit(EXIT_FAILURE);
        }
        flag_pipe = true; // Ustawienie flagi oznaczającej potok 
        // Utworzenie nowego procesu dla każdego polecenia
        pid_t child_pid = fork(); 
        // Blad fork
        if (child_pid < 0) { 
            syslog(LOG_ERR, "Bład utworzenia procesu potomnego %s",strerror(errno));
            exit(EXIT_FAILURE);
        }
        // Proces dziecka
        else if (child_pid == 0) {
            // Jeśli nie jest to pierwsze polecenie w potoku lub jest to pojedyncze polecenie
            if(pipe_command_number != 0){
                // Przekieruj standardowe wejście STDIN na koniec odczytu fd_pipe_in
                if (dup2(fd_pipe_in[0], STDIN_FILENO) == -1) { 
                    syslog(LOG_ERR, "Nie udalo sie wykonac  dup2 dla STDIN: %s\n", head->command);
                    exit(EXIT_FAILURE);
                }
            }
            close(fd_pipe_in[0]); // Zamkniecie deskryptora do odczytu fd_pipe_in
            // Przekieruj standardowe wyjście STDOUT na koniec zapisu stdout_pipe
            if (dup2(stdout_pipe[1], STDOUT_FILENO) == -1) { 
                syslog(LOG_ERR, "Nie udalo sie wykonac  dup2 dla STDOUT: %s\n", head->command);
                exit(EXIT_FAILURE);
            }
            // Przekieruj standardowe wyjście błędów STDERR na koniec zapisu stderr_pipe
            if (dup2(stderr_pipe[1], STDERR_FILENO) == -1) { 
                syslog(LOG_ERR, "Nie udalo sie wykonac  dup2 dla STDERR: %s\n", head->command);
                exit(EXIT_FAILURE);
            }
            // Wykonaj polecenie
            if (execvp(head->command, args) < 0) {
                // Jeśli błąd wykonania polecenia 
                // Wypisz informacje na stderr (informacja zapisze się do potoku stderr_pipe)
                fprintf(stderr,"BLAD_EXEC %s: %s\n",head->command,strerror(errno));
                exit(EXIT_FAILURE); // Zakończ proces
            }
        }
        // Proces macierzysty
        else { 
            waitpid(child_pid, &status, 0); // Czekaj na wykonanie procesu dziecka child_pid
            // Zamknij strumienie do odczytu dla stdout_pipe i stderr_pipe
            close(stdout_pipe[1]);
            close(stderr_pipe[1]);
            if(head->next == NULL){ // Jeśli nie istnieje następne polecenie zakończ pętle (bez tego warunku nie wykonuje sie ostatnie polecenie)
                break;
            }
            pipe_command_number++; // Zwieksz licznik poleceń w potoku  
            // Jeśli polecenie nie było w potoku (np. ls -al) to head->pipe==0    
            if (!head->pipe) { 
                flag_pipe = false; // Ustaw flage na false tym samym konczac petle while
                //break; - prawdopodobnie powoduje błąd
            }
            // Jeśli następne polecenie także znajduje sie w potoku i jest to ten sam potok
            else if(head->next->pipe > 0 && (head->next->pipe == head->pipe)){
                // Sprawdzenie czy wystąpił błąd podczas przetwarzania potoku poleceń (czy stderr_pipe zawiera cokolwiek)
                int nread;
                if(nread = read(stderr_pipe[0], buf, sizeof(buf))){
                    buf[nread] = '\0';  // Dodaj znak konca napisu
                    break;  // Jeśli zawiera (wsytąpił błąd) kończymy pętlę while czyli dalsze przetwarzanie potoku
                }
                // Utworzenie nowego potoku fd_pipe_in dla kolejnego procesu
                if(pipe(fd_pipe_in) == -1){
                    syslog(LOG_ERR, "Bład utworzenia potoku fd_pipe_in: %s",strerror(errno));
                    exit(EXIT_FAILURE);
                }
                // Skopiuj wynik poprzedniego polecenia na koniec do odczytu fd_pipe_in
                if (dup2(stdout_pipe[0],fd_pipe_in[0]) == -1) { 
                    syslog(LOG_ERR, "Nie udalo sie wykonac  dup2 dla fd_pipe_in: %s\n", head->command);
                    exit(EXIT_FAILURE);
                }
                close(fd_pipe_in[1]);   // Zamknij koniec do zapisu fd_pipe_in
                close(stdout_pipe[0]);  // Zamknij koniec do odczytu stdout_pipe
                close(stderr_pipe[0]);  // Zamknij koniec do oczytu stderr_pipe
                head = head->next;    // Przejdz do następnego elementu (polecenia w potoku)
            }
            // Jeśli było to ostatnie polecenie w potoku    
            else{
                flag_pipe = false;    // Ustaw flage flag_pipe na false i zakoncz petle while
                //break; - prawdopodobnie powoduje błąd 
            }
            
        }
    }
    // WEXITSTATUS w przypadku błędu zwraca 1 dlatego użyto zaprzeczenia, wtedy bład sygnalizowany jest poprzez 0
    syslog(LOG_INFO,"KOD WYJŚCIA %s: %d", head->entire_command, !WEXITSTATUS(status));
    // Zapisz wynik do pliku
    SaveToFile(head->time, head->entire_command, stdout_pipe[0], stderr_pipe[0], head->mode, buf, outfile);
}