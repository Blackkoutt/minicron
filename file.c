#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include "list.h"
#include "file.h"
#include "validation.h"

void Read_From_File(Node **head, char* taskfile){
    char buff[MAX_LINE_LENGTH]; // Bufor do odczytu poleceń z pliku
    FILE *fp = fopen(taskfile, "r");    // Otworz plik w trybie do odczytu

    // Jeśli nie udało się otworzyć pliku zapisz informację o błędzie do logu i zakończ program
    if (fp == NULL) {
        syslog(LOG_ERR, "Błąd otwierania pliku %s: %s\n", taskfile, strerror(errno));
        exit(EXIT_FAILURE);
    }
    int pipes_counter = 1;    // Licznik potoków

    // Pętla czytająca plik
    while (fgets(buff, MAX_LINE_LENGTH, fp)) {
        int pipe; // Służy do przypisania "identyfikatora potoku"
        char *hour, *minute, *command, *mode, *args, *entire_command;
        // Alokacja pamięci na tablicę string do przechowywania argumentów polecenia
        char **args_array = malloc(MAX_ARGS * sizeof(char *));
        // Sprawdzenie poprawności alokacji pamięci
        if(args_array == NULL){
            syslog(LOG_ERR, "Błąd alokacji pamięci");
            exit(EXIT_FAILURE);
        }
        // Alokacja pamięci na tablicę string do przechowywania poleceń z potoku 
        char **commands_array = malloc(MAX_PIPE_LENGTH * sizeof(char *));
        // Sprawdzenie poprawności alokacji pamięci
        if(commands_array == NULL){
            syslog(LOG_ERR, "Błąd alokacji pamięci");
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i < MAX_ARGS; i++) {
            args_array[i] = NULL;   // Ustawienie każdego wskaźnika na NULL przed użyciem
        }
        for (int i = 0; i < MAX_PIPE_LENGTH; i++) {
            commands_array[i] = NULL; // Ustawienie każdego wskaźnika na NULL przed użyciem
        }
        // Podział odczytanej linii względem ":" aby uzykać pojedyncze elementy
        hour = strtok(buff, ":");
        minute = strtok(NULL, ":");
        command = strtok(NULL, ":"); // Tutaj całość komendy np. (ls -l|wc -c)
        mode = strtok(NULL, ":");
        // Ze względu na błąd strdup przy podaniu wartości NULL
        // Jeśli po podzieleniu polecenie (command) jest równe NULL
        if(command == NULL){
            entire_command = NULL;    // Do całości komendy przypisujemy NULL (entire_command wykorzystywane w funkcji Validate)
        }
        else{
            entire_command = strdup(command); // Duplikujemy string command na entire_command
        }
        // Jeśli wczytane zadanie jest nieporawnie zdefiniowane
        if(!Validate(hour, minute, entire_command, mode)){
            // Pominięcie tego zadania i zapisanie informacji o tym do logu
            syslog(LOG_INFO, "Komenda ta została pominięta ze względu na błędy w definicji");
            continue;
        }
        // Podział command względem "|" aby uzyskać pojedyncze polecenia z potoku
        int i = 0;
        commands_array[i] = strtok(command,"|");
        i++;
        // Dziel opóki "i" jest mniejsze od maksymalnej ilości poleceń w potoku lub dopóki nie uzyskana zostanie wartość NULL
        for(i = 1; i < MAX_PIPE_LENGTH - 1 && commands_array[i-1] != NULL; i++)
        {
            commands_array[i] = strtok(NULL, "|");
        }
        // Jeśli "i" większe od 2 czyli występuje potok conajmniej 2 poleceń
        if(i > 2){
            pipe = pipes_counter; // Przypisanie identyfikatora potoku
            pipes_counter++; // Zwiększenie licznika potoków 
        }
        else{ pipe = 0; } // Pojedyncze polecenie - brak potoku (identyfikator 0)
        int j = 0;
        // Podział linii względem " " w celu uzyskania argumentów polecenia
        // Dopóki commands_array nie zawiera NULL
        while(commands_array[j] != NULL){
            command = strtok(commands_array[j], " "); // Po pierwszym podziale z strtok zwracane jest polecenie
            i = 0;
            // W kolejnych podziałach zwracane są argumenty zapisywane do tablicy args_array
            args_array[i] = strtok(NULL, " ");
            for(i=1; i<MAX_ARGS-1 && args_array[i-1] != NULL; i++)
            {
                args_array[i] = strtok(NULL, " ");
            }
            // Dodanie nowego elementu do listy (pojedyncze polecenie z argumentami)
            insert_node(head, hour, minute, command, args_array, mode, pipe, entire_command); 
            j++;
        }
    }
    fclose(fp); // Zamknięcie pliku
}
void clearFile(char* outfile){
    FILE *fp = fopen(outfile, "w"); // Otwórz plik outfile w trybie do zapisu (powoduje to wyczyszczenie pliku)
    // Jeśli nie udało się otworzyć pliku
    if (fp == NULL) {
        syslog(LOG_ERR, "Błąd otwarcia pliku %s: %s\n", outfile, strerror(errno));  // Zapisz informacje o błędzie do logu
        return;
    }
    // Jeśli plik udało sie otworzyć
    else{
        syslog(LOG_INFO,"Wyczyszczono plik wynikowy: %s",outfile);  // Zapisz informacje o poprawnym wyczyszczeniu pliku do logu
    }
    fclose(fp); // Zamknij plik
}
void SaveToFile(char* time,char* entire_command, int pipe_out, int pipe_err, int mode, char* error_in_pipe, char* outfile){
    // Otwarcie pliku w trybie "dopisywania"
    FILE *fp = fopen(outfile, "a");
    // Jeśli nie udało sie otworzyć pliku
    if (fp == NULL) {
        syslog(LOG_ERR, "Błąd otwarcia pliku %s: %s\n", outfile, strerror(errno));  // Zapisz informacje o błędzie w logu 
        exit(EXIT_FAILURE); // Zakończ program ze względu na brak możliwości zapisywania wyników
    }
    char buff[SIZE_BUF_RESULT];    // Bufor do odczytu i zapisu danych z potoków stdout_pipe i stderr_pipe
    int nread;
    // Flagi mówiące o braku wyniku
    bool outResultExist = false;
    bool errResultExist = false;

    // Tryb polecenia "wynik na STDOUT" lub "wynik na STDOUT i STDERR"
    if(mode == 0 || mode == 2){
        fprintf(fp,"\n%s Wynik z STDOUT dla polecenia: %s:\n",time,entire_command);
        // Pętla odczytująca z pipe_out (potok stdout_pipe) i zapisująca wynik do pliku
        while(nread = read(pipe_out, buff, sizeof(buff)))   
        {
            outResultExist = true;    // Ustawienie flagi na true - wynik istnieje
            if (nread > 0) {
                buff[nread] = '\0'; // Dodaj znak konca napisu
                fprintf(fp,"%s", buff); // Zapisz wynik do pliku
            }
        }
        // Jeśli wynik z STDOUT nie istnieje
        if(!outResultExist){
            fprintf(fp,"Brak wyniku z STDOUT\n");
        }
    }

    // Tryb polecenia "wynik na STDERR" lub "wynik na STDOUT i STDERR"
    if(mode == 1 || mode == 2){
        fprintf(fp,"\n%s Wynik z STDERR dla polecenia: %s:\n",time,entire_command);
        // Wystąpił błąd w czasie przetwarzania potoku (error_in_pipe zawiera jakąś wartość)
        if(*error_in_pipe != '\0')
        {
             fprintf(fp,"%s", error_in_pipe); // Zapisz wynik do pliku
        }
        // Wystąpił błąd przy wykonywaniu pojedyńczego polecenia lub ostatniego polecenia w potoku
        else{
            // Pętla odczytująca z pipe_err (potok stderr_pipe) i zapisująca wynik do pliku
            while(nread = read(pipe_err, buff, sizeof(buff)))
            {
                errResultExist = true;    // Ustawienie flagi na true - wynik istnieje
                if (nread > 0) {
                    buff[nread] = '\0'; // Dodaj znak konca napisu
                    fprintf(fp,"%s", buff); // Zapisz wynik do pliku
                }
            }
            // Jeśli wynik z STDERR nie istnieje
            if(!errResultExist){
                fprintf(fp,"Brak wyniku z STDERR\n");
            }
        }
        
    }
    fclose(fp); // Zamknij plik
}