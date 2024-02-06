#!/usr/bin/env python3
#python3 script1.py
import datetime
import os

current_dir= os.getcwd()    # Pobranie aktualnej ścieżki w której znajduje sie skrypt
taskfile = os.path.join(current_dir, 'script_taskfile2.txt')    # Złączenie ścieżki i nazwy pliku wejściowego
outfile = os.path.join(current_dir, 'script_outfile2.txt')  # Złączenie ścieżki i nazwy pliku wynikowego

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
    'ls          -l|         wc     -c::: :::::2',
    'ls          -l||||         wc     -c|2*** :2',
    f"cat {os.path.join(current_dir, 'example2.txt')}|sort|uniq -c|sort -nr:0",
    f"cat {os.path.join(current_dir, 'example.txt')}|tr 'a-z' 'A-Z'|grep -i lorem|cat:0",
    'lsss:2',
    'ls -l| :2',
    ' :2',
    "ls          -l| |wc     -c:2",
    "ls -l| :2",
    "ls          -l|         wc     -c:2",
    "chmod 999 file:1",
    "/snappp/bin/firefox:2",
    "ls -l|greap|wc -c:1",
    'ls -al|ls -3:2',
    'tail -n 5 /var/log/syslog:2',
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