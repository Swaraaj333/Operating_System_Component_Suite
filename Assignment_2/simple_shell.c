#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/time.h>

// creates array of struct for storing history
typedef struct {
    char command[2000];
    pid_t pid;
    struct timeval start_time;
    double execution_time;

} Command_History;

Command_History history[400];

int history_counter = 0;

// taking input and making sure no trail spaces and newlines 
char *Input() {
    char *input_command =(char *)malloc(400);
    if (input_command==NULL) {
        printf("Memory allocation failed\n");
        return NULL;
    }

    if (fgets(input_command,200,stdin) == NULL) {

        return NULL;
    }

    int command_length= strlen(input_command);
    for (int i =command_length - 1;i >= 0; i--) {
        if (input_command[i] == ' '||input_command[i] == '\n') {
            input_command[i] = '\0';  
        } else {
            break;
        }
    }

    return input_command;
}

// Parsing the input command breaking it and making a list of it and ending list will NULL
char **Input_Parser(char *user_input) {

    char **command_list = malloc(200 * sizeof(char *));
    int i = 0;
    char *list_elts;

    list_elts = strsep(&user_input, " ");
    while (list_elts != NULL) {
        if (strlen(list_elts) > 0) {

            command_list[i] = strdup(list_elts);
            i++;
        }
        list_elts = strsep(&user_input," ");
    }

    command_list[i]= NULL;  
    return command_list;
}

//This helps in running commands with no pipe
void Run_Command(char **cmd_list, char *input) {
    if (history_counter >= 400) {
        printf("History limit exceeded \n");
        return;
    }

    pid_t pid;
    struct timeval t_start,t_end;
    gettimeofday(&t_start,NULL);

    pid = fork();
    if (pid == 0) {  
        execvp(cmd_list[0], cmd_list);
        printf("Unable to execute");
        exit(1);
    } else if (pid > 0) {  

        wait(NULL);
        gettimeofday(&t_end, NULL);
        // storing all the info required for printing history
        double time_vsec = t_end.tv_sec - t_start.tv_sec;
        double time_usec = t_end.tv_usec - t_start.tv_usec;
        double time_use = time_vsec + time_usec / 1000;

        if(strlen(input) < sizeof(history[history_counter].command)) 
        {
            strcpy(history[history_counter].command, input);
        } else{
            history[history_counter].command[0] = '\0';
        }

        history[history_counter].start_time = t_start;
        history[history_counter].execution_time = time_use;
        history[history_counter].pid = getpid();

        history_counter++;

    } else {
        printf("Unable to perform fork\n");
    }
}

// this helps in running command with pipes
void Run_Pipeline(char ***c_list, int n, char *input) {
    if (history_counter >= 400) return;

    int i;
    int in_fd = 0;
    int pipefd[2];
    pid_t pid;
    struct timeval p_start, p_end;

    gettimeofday(&p_start, NULL);

    for(i = 0; i < n; i++) {
        if(i < n - 1) {
            if (pipe(pipefd) == -1) {
                perror("Unable to pipe");
                return;

            }
        }

        pid = fork();

        if(pid == -1) {
            perror("Fork did not happen");
            return;
        }

        if(pid == 0) {  // Child process
            if (in_fd != 0) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }

            if(i < n - 1) {
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[0]);
                close(pipefd[1]);
            }

            execvp(c_list[i][0], c_list[i]);
            perror("Execution failed");
            exit(1);
        }else {  // Parent process
            if(in_fd != 0) close(in_fd);

            if(i < n - 1) {
                close(pipefd[1]);
                in_fd = pipefd[0];
            }
        }
    }

    for(i = 0; i < n; i++){ 
        wait(NULL);
    }

    gettimeofday(&p_end, NULL);
    // scalculating the duration and saving it in history to print
    double time_vsec = p_end.tv_sec - p_start.tv_sec;
    double time_usec = p_end.tv_usec - p_start.tv_usec;
    double time_use = time_vsec + time_usec / 1e6;

    if(strlen(input) < sizeof(history[history_counter].command)) {
        strcpy(history[history_counter].command, input);

    }else {
        history[history_counter].command[0] = '\0';
    }

    history[history_counter].pid = getpid();
    history[history_counter].start_time = p_start;
    history[history_counter].execution_time = time_use;

    history_counter++;
}

// this history is when history command is given by user only history gets printed no time
void History() {
    for (int i = 0; i < history_counter; i++) {
        printf("%s\n", history[i].command);
    }
}

// when user does ctrl c the time and details get printed
void History_exit() {
    for (int i = 0; i < history_counter; i++) {

        char *s_time = ctime(&history[i].start_time.tv_sec);
        s_time[strcspn(s_time, "\n")] = '\0'; 

        printf("command: %s, pid: %d, start time: %s, duration: %f seconds\n",

               history[i].command,
               (int)history[i].pid,
               s_time,
               history[i].execution_time);
    }
}

// this handles the ctrl c
void sig_int(int sig) {
    printf("\n Ctrl-C Entered, Shell Exit \n");
    History_exit();
    exit(0);
}

// launches the code and handles pipe and non pipe commands 
int Launch(char *input) {
    if (strcmp(input, "history") == 0) {
        History();
        return 1;
    }

    if (strcmp(input, "time") == 0) {
        time_t t = time(NULL);
        printf("Current time is: %s", ctime(&t));
        return 1;
    }

    int n = 0;
    char *temp = strdup(input);
    char *com;
    char *parts[50];

    while((com = strsep(&temp,"|")) != NULL) {
        parts[n] = strdup(com);
        n++;
    }

    free(temp);
    
    if(n > 1) {
        char **cmd_list[n];
        for(int i = 0; i < n; i++) {
            cmd_list[i] = Input_Parser(parts[i]);
        }

        Run_Pipeline(cmd_list, n, input);

        for(int i = 0; i < n; i++){
            free(cmd_list[i]);
        }

    }else {
        char **cmd = Input_Parser(input);
        Run_Command(cmd, input);
        free(cmd);
    }

    for (int i = 0; i < n; i++) {
        free(parts[i]);
    }

    return 1;
}

// shell code  , user can give commands here and it handles it 
int main() {
    signal(SIGINT, sig_int);  

    char *input;

    do {
        printf("swaraaj@iiitd$ ");
        input = Input();

        if(input == NULL) continue;

        if(strcmp(input, "exit") == 0) {
            History_exit();
            printf("Shell has been exited.\n");
            free(input);
            break;
        }

        if(strlen(input) > 0) {
            Launch(input);
        }

        free(input);
    } while (1);

    return 0;
}
