# Table Of Content

- [General info](#general-info)
- [Technologies](#technologies)
- [Getting Started](#getting-started)
    - [Usage of daemon](#usage-of-daemon)
    - [Running test scripts](#running-test-scripts)
    - [Additional signal options](#additional-signal-options)
    - [Usage of syslog](#usage-of-syslog)

# General info
The project is a custom implementation of the minicron system daemon dedicated to Linux. Here is a list of available functionality:

- Validation of the given program arguments. If the arguments are invalid
the program terminates by printing error information on the STDERR
- The program runs in the background
- Loading tasks from the input file given as a parameter into an unidirectional list
- Validating the syntax of the given tasks to prevent unexpected errors.
If the task has incorrect syntax, it is not considered (not placed on the list). Information about failures during task loading and their reasons is displayed in the system log
- Sorting of loaded tasks chronologically by their execution time. For
Sorting uses the Merge Sort algorithm
- The daemon executes a task when it reaches its scheduled execution time, as defined by the user in the input file. Otherwise, the daemon remains idle
- Signal handling. During the execution of the daemon, it is possible to send 4
control signals:
  - SIGINT - terminates the daemon
  - SIGUSR1 - reload tasks from the specified file to the list
  - SIGUSR2 - write tasks remaining to be executed in the system log
  - SIGQUIT - clears the resulting file
- Support for pipelines, e.g. (ls -l|wc -c) with a maximum length of 10 commands in the
pipeline. The length of the pipeline can be set arbitrarily via the #define directive
MAX_PIPE_LENGTH in the file.h file. Each command in the pipeline is executed
by a new process. The processes communicate with each other by writing the
result to the stdout_pipe pipeline and reading the previous result from the pipeline
fd_pipe_in.
- Supports commands that have up to 10 arguments. The number of
arguments can be freely set via the #define MAX_ARGS directive
in the files file.h and list.h (you should change the value in both files to avoid
unexpected errors).
- Write the result of the command to the file given as the second argument. The output
information depends on the given command execution mode. You can get
information from STDOUT (mode 0), STDERR (mode 1) or STDOUT and STDERR (mode 2).
- Information about the start of task execution and its output code are
placed in the system log. Whereby the exit code is interpreted as:
  - 0 - an error occurred during the execution of the command
  - 1 - the command was executed correctly
- A simple makefile to compile the program, as the program consists of many
files
- Two test scripts written in Python (the script compiles and
runs the minicron program)

# Technologies
The project was written using the following technologies:

![Static Badge](https://img.shields.io/badge/C%20programming%20language-%23004283?style=for-the-badge&logo=C)

![Static Badge](https://img.shields.io/badge/Python-%233874a4?style=for-the-badge&logo=python&logoColor=%23ffe15c)

![Static Badge](https://img.shields.io/badge/Linux%20API-%23000000?style=for-the-badge&logo=linux&logoColor=%23000000&labelColor=%23ffe15c)

# Getting Started


### Usage of daemon

- You need to create a file with extension .txt (the file can be created in any
directory), in which you need to define the commands executed by the daemon
Syntax is: **HH:MM:command:mode**
  - **e.g. 12:30:ls -l:2**
  - **or if using streams, e.g. 12:30:ls -l|wc -c:2**
  - Alternatively, you can use the ready attached file "godziny.txt" and
set the hours of execution of tasks according to your needs.
- Next, compile the daemon code using the makefile. To do this
in the console you need to go to the directory where all the files of the
of the program along with the makefile. **After doing this, all you need to do is
type the command "make" in the console**
- **The last step is to run the daemon using ./minicron taskfile outfile.**
- To check if the daemon has been started correctly it is worth checking,
whether the daemon process was started by typing in the console the command **"ps -aux|grep minicron"**

> [!WARNING]
> **In order to avoid errors opening the file, you need to specify the full path of the
both files, e.g. /home/student/taskfile.txt.**



### Running test scripts

In order to test the operation of the daemon, 2 test scripts were written in Python.
- The script1.py file contains commands that should execute correctly. 
- The script2.py file contains both commands that should execute correctly and
those incorrectly declared, which should cause errors to be printed to the
STDERR in the resulting file or the omission of some tasks due to errors in the
definition (in this situation, check the system log).

Each script creates an input file "script_taskfile[script number].txt" for the
minicron program in which it stores task information, and an output file
"script_outfile[script number].txt". After that, it compiles the minicron program with a
makefile and runs it with the created files given as arguments.
All the declared tasks should be executed within 3 min from the
running the script.

**To run the test scripts you must only type ./script1.py or ./script2.py in your console window depending on wich script you want to run**

> [!NOTE]
> The scripts use the python version 3 language interpreter.

> [!TIP]
> To run the script you need to check whether it has the permissions to execute, if it does not have them, you should grant them using "chmod +x command
script[no_script].py" , and then type ./script1.py or ./script2.py in the console. (O
as long as the user is in the directory with the script).


### Additional signal options

In order to send any signal to the daemon, it is necessary at first to check the
daemon process identifier (pid). This can be done with the command "ps -
aux|grep minicron" (the second column contains the pid of the daemon process).

- kill -QUIT <PID> - sends a SIGQUIT signal, which will clear the file
resultant
- kill -USR1 <PID> - sends a SIGUSR1 signal that will re
loading of the tasks to be executed
- kill -USR2 <PID> - sends a SIGUSR2 signal that will write the remaining
tasks to be executed to the system log
- kill -INT <PID> - sends a SIGINT signal that will terminate the
daemon

### Usage of syslog

To check the information generated by the daemon, type in the console
command:
   ```
    less /var/log/syslog|grep Mini-cron
   ```
