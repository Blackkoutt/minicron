#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <syslog.h>
#include "validation.h"

void argsValidate(int argc, char* first_file, char* second_file){
    // Walidacja liczby argumentów
    if(argc != 3)
    {
        fprintf(stderr,"Błędna liczba argumentów\n");
        exit(EXIT_FAILURE);
    }
    // Sprawdzenie czy oba podane argumenty (pliki) mają rozszerzenie .txt
    // Z ".txt" porównywany jest podany ciąg znaków, przeusnięty "w prawo" o jego długość - 4 co powinno także dać ciąg ".txt"
    if (strcmp(first_file + strlen(first_file) - 4, ".txt") != 0 || strcmp(second_file + strlen(second_file) - 4, ".txt") != 0) {
        fprintf(stderr,"Podano plik z innym rozszerzeniem niż .txt\n");
        exit(EXIT_FAILURE); // Zakończenie programu
    }
    // Sprawdzenie czy plik zwierający polecenie istnieje i czy można go odczytać (czy mamy prawo do odczytu)
    if (access(first_file, F_OK) == -1){
        fprintf(stderr,"Podany plik nie istnieje lub użytkownik nie ma do niego praw dostępu\n");
        exit(EXIT_FAILURE); // Zakończenie programu
    }
    // Jeśli żaden warunek nie został spełniony podane argumenty sa poprawne 
    return;
}
bool Validate(char* hour, char* minute, char* command, char* mode){
    // Sprawdzenie czy podano wszystkie wartości
    if(hour == NULL || minute == NULL || mode == NULL || command == NULL){
        syslog(LOG_ERR, "Jedna z komend nie została poprawnie zadeklarowana w pliku (brak pewnych wartości)");
        return false;
    }
    // Tutaj trzeba sprawdzic dlugosc bo potem odwolujemy sie pod 1 index tablicy (2 element)
    if(strlen(hour) != 2 || strlen(minute) != 2){
        syslog(LOG_ERR, "W pliku podano bledny czas wykonania komendy: %s", command);
        syslog(LOG_INFO, "Czas musi być zapisany w postaci HH:MM");
        return false;
    }
    // Sprawdzenie czy godzina i minuta są liczbami
    if(!(isdigit(hour[0]) && isdigit(hour[1]) && isdigit(minute[0]) && isdigit(minute[1]))){
        syslog(LOG_ERR, "W pliku podano bledny czas wykonania komendy: %s", command);
        syslog(LOG_INFO, "Czas musi być wartością numeryczną z przedziału [00:00 - 23:59]");
        return false;
    }
    // Sprawdzenie czy tryb jest liczbą
    if(!isdigit(mode[0])){
        syslog(LOG_ERR, "W pliku podano bledny tryb wykonania komendy: %s", command);
        syslog(LOG_INFO, "Tryb musi być wartością numeryczną z przedziału [0 - 2]");
        return false;
    }

    // Konwersja char* na typ int 
    // atoi zwraca 0 w przypadku bledu i 0 w przypadku podania wartosci 0
    // dlatego należało sprawdzić powyższe warunki
    int hour_num = atoi(hour);
    int minute_num = atoi(minute);
    int mode_num = atoi(mode);

    // Sprawdzenie czy godzina jest poprawna [00:00 - 23:59]
    if(hour_num < 0 || hour_num > 23 || minute_num < 0 || minute_num > 59){
        syslog(LOG_ERR, "W pliku podano bledny czas wykonania komendy: %s", command);
        syslog(LOG_INFO, "Należy podać czas z przedziału [00:00 - 23:59]");
        return false;
    }
    // Sprawdzenie czy tryb jest poprawny [0 - 2]
    if(mode_num !=0 && mode_num !=1 && mode_num !=2 ){
        syslog(LOG_ERR, "W pliku podano bledny tryb wykonania komendy: %s", command);
        syslog(LOG_INFO, "Jako tryb zadania można podać wartość 0, 1 lub 2");
        return false;
    }

    // Sprawdzenie poprawności podanej komendy
    bool onlySpaces;    // Flaga informująca o spacjach
    bool empty = true;    // Flaga do rozpoznania czy cokolwiek znajduje się w podanej komendzie
    // Przechodzimy po całym stringu command 
    for(int i = 0; i < strlen(command); i++){
        // Jeśli w danym miejscu występuje spacja i do tej pory nic nie znaleziono (empty == true)
        // Może wystąpić sytuacja typu "ls -l      " wtedy komenda powinna się wykonać dlatego potrzebna jest flaga empty
        if(isspace(command[i]) && empty){
            onlySpaces = true; // Ustawiaj flagę onlySpaces na true (są tylko spacje)
            // Jeśli następny element tablicy nie istnieje i mamy same spacje zwróć false (błąd składni)
            if(i+1 >= strlen(command)){
                syslog(LOG_ERR, "Komenda '%s' nie została poprawnie zadeklarowana w pliku (bład składni)",command);
                return false;
            }
        }
        // Jeśli aktualnym elementem tabicy jest znak potoku "|"
        else if ((command[i] == '|')){
            // Jeśli do tej pory flaga onlySpaces była true oznacza to że mamy sytuację typu "   |ls -l" lub "ls -l| |wc -c"
            // Taka sytuacja może powodować błędy w wykonaniu dlatego zwracane jest false (błąd składni)
            if(onlySpaces){
                syslog(LOG_ERR, "Komenda '%s' nie została poprawnie zadeklarowana w pliku (błąd składni)",command);
                return false;
            }
            // Jeśli nie było pustego polecenia ustaw empty na true tym samym kontynuując wykonanie pętli np. ls -l| |wc -c
            empty = true; 
        }
        // Jeśli wykryto cokolwiek w poleceniu (nie jest to spacja)
        else{
            empty = false; // Ustaw flagę empty na false tym samym zablokuj wykonanie pierwszego warunku if
            onlySpaces = false; // Ustaw flagę onlySpaces na false blokując wyrzucenie błędu w drugim warunku else if
        }
    }
    // Jeśli wszystko jest poprawnie zwróć true
    return true;
}