### Summary ###
This code is used for an operating system class. The functionalities we tried to implement include: redirection, pipe, job control, signal handling, terminal control and so on.

#### Authors: ####
Morgan and Jimmy

#### Compilation Instructions: ####
run `make` in the same directory to compile the program<br>
run `./kinda-sh` to start the program and ctrl+D or ctrl+\ to terminate<br>
run `make clean` to remove the executables and object files<br>

#### Code Layout ####
The main() function is inside the kinda-sh.c file. The list structure is declared in the linkedList.h and implemented in linkedList.c. This list structure is used for storing the information about jobs, such as pgid, the name of the job, the status of the jobs and so on. There are also some functions to modify the list and search the list. <br>
In kinda-sh.c, we have some utility functions above the main() function. Those utility functions deal with argument parsing, string-integer translation, getting the name of jobs, user input validation. We also have functions to deal with asynchronous signal handler. Basically, all the SIGCHLD signals are handled in the sig_handler() function, and the report of their status changes are inside the check_gp_status(), and we call this function at the beginning of each loop before the shell prompt, so the status change report will not interrupt the user. <br>
Inside the main() function, we did some initializations such as blocking certain signals for the shell, assign values to global variables. Inside the while loop, there is argument parsing and argument validation. After that, the program is divided into two parts: input has no pipe vs input has pipe(s). Since we are doing multi stages pipe, the second part is decided into 3 sections: the first and the last program in the pipe are executed separately and the 2nd to the second last programs are executed in a loop. In all the cases, we have checked whether the job is executed initially in foreground or background, so that we can determine the parent's(the shell's) behavior.<br>
The sig_handler, inside the kinda-eh.c file, would be invoked when SIGCHLD interrupts. The handler would gather the status of the child process and determine what causes the child process to invoke SIGCHLD. Then handles each case accordingly.
