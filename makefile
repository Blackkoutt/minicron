objects = minicron.o file.o validation.o command.o list.o #lista plików obiektowych

minicron: $(objects) #plik wykonywalny minicron jest tworzony z listy plikow obiektowych
	gcc $(objects) -o minicron

file.o: file.c file.h validation.h list.h #plik obiektowy file.o jest tworzony z plikow file.c file.h validation.h list.h
	gcc -c file.c -o file.o

validation.o: validation.c validation.h #plik obiektowy validation.o jest tworzony z plikow validation.c validation.h
	gcc -c validation.c -o validation.o

command.o: command.c command.h file.h list.h #plik obiektowy command.o jest tworzony z plikow command.c command.h file.h list.h
	gcc -c command.c -o command.o

list.o: list.c list.h #plik obiektowy list.o jest tworzony z plikow list.c list.h
	gcc -c list.c -o list.o

.PHONY: clean #elimunuje bledy w przypadku gdy w katalogu znajduje sie takze plik o nazwie clean

#clean usuwa plik wykonywalny program oraz wszystkie stworzone pliki obiektowe
clean:
	rm minicron $(objects)