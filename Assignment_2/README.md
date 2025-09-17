#SimpleShell#


SimpleShell is a simple version of shell program in C that works like an actual Shell. It lets us run normal commands and also commands connected with pipes. It keeps a history of all commands with details like process ID, start time, and how long each command took.

#Header Files Used#
stdio.h – For input and output functions like printf() and fgets().
stdlib.h – For memory and process functions like malloc(), free(), and exit().
unistd.h – For system calls like fork(), execvp(), and pipe().
sys/types.h – For data types like pid_t.
string.h – For handling strings with functions like strcpy(), strdup(), strlen(), and strsep().
signal.h – To handle signals like Ctrl+C.
sys/wait.h – To wait for child processes using wait().
time.h and sys/time.h – To get the current time and measure how long commands take.

#Functions#
Input() – Reads input from the user, removes extra spaces and newlines, and returns a clean string.

Input_Parser() – Splits the input into parts (arguments) that execvp can use.

Run_Command() – Runs a single command. It creates a child process, runs the command, and saves its details in history.

Run_Pipeline() – Runs commands connected by pipes. It sets up pipes, runs multiple child processes, connects their input and output, and saves the details in history.

History() – Shows a simple list of commands entered.

History_exit() – Shows full details of commands, including PID, start time, and duration. Called when you exit the shell or press Ctrl+C.

sig_int() – Handles Ctrl+C, shows full command history, and exits the shell.

Launch() – Checks if the input is a built-in command like exit, history, or time. If there are pipes, it calls 
Run_Pipeline(); otherwise, it calls Run_Command().

main() – Shows the shell prompt swaraaj@iiitd$, reads user input continuously, handles built-in commands, and uses Launch() to run other commands.

nano – SimpleShell cannot run nano. Nano is a text editor that needs full control of the terminal, including typing, screen updates, and moving the cursor. SimpleShell only handles basic input and output, so nano does not work.

vim – SimpleShell also cannot run vim. Vim needs real-time typing, cursor movement, and special terminal modes. SimpleShell cannot provide these features, so vim will not work properly.

top / htop – SimpleShell cannot run top or htop. These programs show live updating information about the system, like CPU and memory use. SimpleShell only supports simple commands and piping, so it cannot handle programs that need continuous updates and interactive screens.
	


Work Division

1)Prerak Tanwar 2024436
a)Input() function
b)Run_Command() function
c)History() function
d)History_exit() function

2)Swaraaj Krishna 2024574
a)Input_parser()
b)Run_pipeline()
c)sig_int()
d)Launch()
e)Main()

Debugging and Testing was done together.
Code Citation

