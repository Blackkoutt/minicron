#ifndef COMMAND_H
#define COMMAND_H

// extern informuje że definicje poniższych zmiennych znajdują się w innym miejscu
extern volatile bool flag_end;
extern volatile bool flag_save;
extern volatile bool flag_reload;
extern volatile bool flag_clear;

void executeCommand(char* outfile);

#define SIZE_BUF_OUT 1024 // Wielkosc bufora do odczytu

#endif