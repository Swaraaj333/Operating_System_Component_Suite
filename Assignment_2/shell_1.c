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
