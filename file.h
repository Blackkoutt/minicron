#ifndef FILE_H
#define FILE_H

#define MAX_LINE_LENGTH 1024    // Dlugość bufora do odczytu z pliku
#define SIZE_BUF_RESULT 1024    // Wielkosc bufora do odczytu
#define MAX_ARGS 10 // Maksymalna ilosc argumentow dla polecenia
#define MAX_PIPE_LENGTH 10  // Maksymalna ilość poleceń w potoku

void Read_From_File(Node **head, char* taskfile);
void SaveToFile(char* time, char* entire_command, int pipe_out, int pipe_err, int mode, char* error_in_pipe, char* outfile);
void clearFile(char* outfile);

#endif