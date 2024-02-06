#ifndef LIST_H
#define LIST_H

#define MAX_ENTIRE_COMMAND_LENGTH 1024  // Maksymalna dlugosc calej komendy
#define MAX_ARGS 10 // Maksymalna ilosc argumentow dla polecenia

typedef struct Node {
    char time[6];   // Czas wykonania polecenia
    char command[100];  // Pojedyncze polecenie do wykonania
    char entire_command[MAX_ENTIRE_COMMAND_LENGTH]; // Całość polecenia 
    char* args[MAX_ARGS]; // Tablica argumentów podanych do polecenia
    int argc; // Ilość podanych argumentów
    int mode; // Typ operacji (skąd ma się pojawić wynik: stdout, stderr, to i to)
    int time_diff; // Rożnica czasu (potrzebna do sortowania)
    int pipe; // Czy polecenie jest w potoku (wartość > 0 oznacza że polecenie jest w potoku)
    struct Node *next; // Następny element
} Node;

extern Node *head; // extern oznacza że definicja znajduje się w innym miejscu 

void insert_node(Node **head, char* hour, char* minute, char* command, char** args_array,char* mode, int pipe, char* entire_command);
void delete_list(Node **head);
void Save_To_Log(Node *head);
Node *merge(Node *left, Node *right);
void split(Node *head, Node **left, Node **right);
void mergesort(Node **head);
int time_difference(char *time1, char *time2);

#endif