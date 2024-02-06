#!/usr/bin/env python3
#python3 script1.py
import datetime
import os

current_dir= os.getcwd()    # Pobranie aktualnej ścieżki w której znajduje sie skrypt
taskfile = os.path.join(current_dir, 'script_taskfile1.txt')    # Złączenie ścieżki i nazwy pliku wejściowego
outfile = os.path.join(current_dir, 'script_outfile1.txt')  # Złączenie ścieżki i nazwy pliku wynikowego

# Czyszczenie obu plików jeśli były już utworzone
with open(taskfile, 'w') as file:
    file.truncate(0)

file.close()
with open(outfile, 'w') as file:
    file.truncate(0)

file.close()

# Pobranie aktualnego czasu
current_time = datetime.datetime.now()

# Lista zadań do wykonania
tasks = [
    'ls -al|wc -c:2',
    'ps -aux:0',
    'ps -aux|grep root:2',
    'ps -aux|grep root|wc -c:0',
    'ls -l /sys/devices/system/container/power:2',
    f"rm -rf {os.path.join(current_dir, 'testDIR')}:2",
    f"mkdir {os.path.join(current_dir, 'testDIR')}:2",
    f"touch {os.path.join(current_dir, 'testDIR/plik.txt')}:2",
    f"ls -l {os.path.join(current_dir, 'testDIR')}:2",
    f"chmod 777 {os.path.join(current_dir, 'testDIR/plik.txt')}:2",
    f"ls -l {os.path.join(current_dir, 'testDIR')}:2",
    'whereis firefox:2',
    '/snap/bin/firefox:2',
    'ls -al|grep 4096|wc -c:2',
    'ps -aux|wc -c:2',
]

# Zapis zadań do pliku taskfile
i=0
with open(taskfile, 'a') as file:
    for task in tasks:
        # Co 5 zadanie zwiększamy czas wykonania o aktualny+1min
        if i % 5 == 0 and i != 0:
            current_time += datetime.timedelta(minutes=1)
        time_str = current_time.strftime('%H:%M') # Konwersja czasu na format tekstowy godzina:minuta
        file.write(f'{time_str}:{task}\n')  # Zapis zadań do pliku
        i += 1

file.close()

os.system('make -f makefile')   # Skompilowanie minicrona za pomocą makefile
os.system(f'./minicron {taskfile} {outfile}')   # Uruchomienie minicrona z utworzonymi plikami