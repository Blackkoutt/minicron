#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <syslog.h>
#include <stdbool.h>
#include "list.h"


void delete_list(Node **head)
{
    Node *temp = *head; // Przypisanie początku listy do zmiennej pomocniczej 
    while (temp != NULL) {
        for (int i = 0; i < temp->argc; i++) {
            free(temp->args[i]); // Zwolnienie pamięci dla każdego argumentu
        }
        Node *next_node = temp->next; // Przypisanie następnego elementu do zmiennej pomocniczej
        free(temp); // Zwolnienie pamięci dla aktualnego elementu listy
        temp = next_node; // Przejście do kolejnego elementu listy 
    }
    *head = NULL; // Ustawienie wartości pierwszego elementu listy na NULL
}
void insert_node(Node **head, char* hour, char* minute, char* command, char **args_array,char* mode,int pipe, char* entire_command) {
    Node *new_node = (Node*)malloc(sizeof(Node));   // Alokacja pamieci dla pojedynczego elementu
    // Sprawdzenie poprawności alokacji pamięci
    if (new_node == NULL) {
        syslog(LOG_ERR, "Błąd alokacji pamięci przy dodawaniu nowego elementu do listy");
        exit(EXIT_FAILURE);
    }
    strcpy(new_node->entire_command, entire_command);   // Skopiowanie całej komendy np(ls -l|wc -c)
    strcpy(new_node->command, command); // Skopiowanie pojedynczej komendy np(ls -l, wc -c)
    int i=0;
    while(args_array[i] != NULL && i < MAX_ARGS){
        new_node->args[i] = malloc(strlen(args_array[i])+1); // Alokacja pamieci dla pojedynczego argumentu
        // Sprawdzenie poprawności alokacji pamięci
        if(new_node->args[i] == NULL){
            syslog(LOG_ERR, "Błąd alokacji pamięci przy dodawaniu nowego elementu do listy");
            exit(EXIT_FAILURE);
        }
        strcpy(new_node->args[i], args_array[i]); // Skopiowanie argumentu do tablicy argumentów danego polecenia
        i++;
    }
    new_node->argc = i;   // Przypisanie ilości argumentów danego polecenia
    new_node->mode = atoi(mode);    // Przypisanie trybu wykonania polecenia
    new_node->pipe = pipe;    // Przypisanie identyfikatora potoku
    char time[6];   // Czas HH:MM + znak końca łańcucha '\0'
    sprintf(time,"%s:%s", hour, minute);    // Złączenie godziny i minut w czas HH:MM
    strcpy(new_node->time, time);   // Skopiowanie czasu wykonania danego polecenia
    new_node->time_diff = time_difference(time, "now"); // Obliczenie różnicy czasu między aktualnym a podanym
    new_node->next = NULL;  // Ustawienie wskaźnika na następny element jako NULL

    // Jeśli początek listy jest NULL to wstaw element na początek
    if (*head == NULL) {
        *head = new_node;
    }
    // Jeśli lista nie jest pusta
    else { 
        Node *current = *head;  // Przypisanie do zmiennej pomocniczej
        while (current->next != NULL) { // Przeszukanie listy do konca
            current = current->next;
        }
        current->next = new_node; // Dodanie elementu na koncu listy
    }
}
void Save_To_Log(Node *head){
    Node* current = head;   // Przypisanie aby nie pracować na oryginale wskaźnika na początek listy
    bool nextExist = true;
    syslog(LOG_INFO,"Pozostałe zadania do wykonania:");
    // Jeśli lista jest pusta zapisz informacje o tym w logu
    if(current == NULL) syslog(LOG_INFO,"Brak zadań do wykonania");
    // Pętla przechodząca po wszystkich elementach listy aż do końca
    while (current != NULL) {
        syslog(LOG_INFO,"%s:%s:%d", current->time, current->entire_command,current->mode); // Zapisanie informacji o poleceniu w logu
        if(current->next == NULL) break;
        // Jeśli polecenie znajduje się w potoku to pomiń pozostałe polecenia w tym potoku
        // Każde pojedyncze polecenie z potoku posiada oddzielny element na liscie 
        while(current->pipe > 0 && (current->pipe == current->next->pipe)){
            current = current->next; 
            if(current->next == NULL){
                nextExist = false;
                break;
            }
        }
        if(!nextExist){ break; }
        current = current->next; // Przejdz do następnego polecenia
    }
}
int time_difference(char *time1, char *time2) {
    time_t t;   // Zmienna t przechowuje czas
    struct tm *now_tm, *time_tm;    // Deklaracja struktury tm co umożliwia odwołanie się do godzin, minut itd.
    int diff;   // Do przechowania różnicy czasu
    time(&t);   // Pobranie aktualnego czasu i zapisanie w zmiennej t
    now_tm = localtime(&t); //  Konwersja na reprezentacje czasu w strukturze tm
    // Obliczenie podanego czasu w minutach i godzinach
    // -'0' aby uzyskać liczbe z kodu ASCII, gdyż 0 ma kod 48, a np 1 ma kod 49
    int hour1 = (time1[0] - '0') * 10 + (time1[1] - '0'); 
    int min1 = (time1[3] - '0') * 10 + (time1[4] - '0');
    // Jeśli drugim argumentem jest aktualny czas (string "now")
    if (strcmp(time2, "now") == 0) {
        // Obliczenie różnicy czasu w minutach
        diff = (hour1 - now_tm->tm_hour) * 60 + (min1 - now_tm->tm_min);
        // Jeśli różnica jest mniejsza od zera dodajemy pełną dobę 
        if (diff < 0) {
            diff += 24 * 60;
        }
    }
    // Jeśli drugim argumentem nie jest aktualny czas
    else {
        // Oblicz godzinę i minutę drugiego argumentu tak jak poprzednio
        int hour2 = (time2[0] - '0') * 10 + (time2[1] - '0');
        int min2 = (time2[3] - '0') * 10 + (time2[4] - '0');
        diff = (hour1 - hour2) * 60 + (min1 - min2);
        // Jeśli różnica jest mniejsza od zera dodajemy pełną dobę 
        if (diff < 0) {
            diff += 24 * 60;
        }
    }
    return diff; // Zwóć różnicę czasu
}
Node *merge(Node *left, Node *right) {
    Node *result = NULL;    // Lista wynikowa
    
    // Sprawdzenie czy któraś z list jest pusta
    if (left == NULL) {
        return right;   // Jeśli lewa pusta zwróć prawą
    } 
    else if (right == NULL) {
        return left;    //Jeśli prawa pusta zwróć lewą
    }

    // Jeśli obie listy zawierają cokolwiek
    // Porównanie różnic czasu dla elementów w obu listach
    // Jeśli element w lewej liście wykona się wcześniej
    if (left->time_diff <= right->time_diff) {
        result = left;  // Do listy wynikowej wpisz element z lewej listy
        // Następny element listy wynikowej będzie zależał 
        // od wartości następnego elementu lewej listy i aktualnego elementu prawej listy
        result->next = merge(left->next, right);    
    } 
    // Jeśli element w prawej liście wykona się wcześniej
    else {
        result = right; // Do listy wynikowej wpisz element z prawej listy
        // Następny element listy wynikowej będzie zależał 
        // od wartości następnego elementu prawej listy i aktualnego elementu lewej listy
        result->next = merge(left, right->next);
    }

    return result;  // Zwróć listę wynikową
}

void split(Node *head, Node **left, Node **right) {
    Node *fast, *slow;
    slow = head;    // Slow to poczatek listy
    fast = head->next;  // Fast to drugi element listy
    // Dopóki nie doszliśmy do końca listy
    while (fast != NULL) {
        fast = fast->next;  //Przesunięcie fast o 1
        // Jeśli nie jest to koniec listy
        if (fast != NULL) {
            slow = slow->next;  // Przesunięcie slow o 1
            fast = fast->next;  // Przesunięcie fast o 1
        }
    }
    // W ten sposób wyznaczamy środek listy i w tym miejscu wykonujemy podział na dwie listy

    *left = head;   // Lewa strona to poczatek pierwszej listy
    *right = slow->next;    // Prawa do srodek listy (początek drugiej)
    slow->next = NULL; // Rozdzielenie list 
}

void mergesort(Node **head) {
    Node *temp = *head; // Przypisanie head do zmiennej pomocniczej temp
    Node *left, *right; // Lewa i prawa lista 

    //Jeśli lista pusta lub ma jeden argument (brak potrzeby sortowania)
    if (temp == NULL || temp->next == NULL) {
        return;
    }
    split(temp, &left, &right); // Podzial listy na dwie podlisty
    mergesort(&left);   // mergesort dla lewej strony listy
    mergesort(&right);  // mergesort dla prawej strony listy
    *head = merge(left, right); // Polaczenie dwóch podlist (sortowanie)
}