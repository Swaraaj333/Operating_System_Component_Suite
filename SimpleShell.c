#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>

typedef struct {
    char command[2000];
    pid_t pid;
    struct timeval t_start;
    double t_finish;
} Command_History;

Command_History history[400];
int history_counter = 0;

int sch_fd;
pid_t sch_pid;
int NCPU, TSLICE;

char *Input() {
    ssize_t n_read;
    size_t buffer_size = 0;
    char *line = NULL;
    int err_no = 0;

    n_read = getline(&line, &buffer_size, stdin);
    if (n_read == -1) {
        if (feof(stdin)) {
            free(line);
            return NULL;
        }
        if (err_no == EINTR) {
            free(line);
            return NULL;
        }
        free(line);
        return NULL;
    }

    while(n_read > 0 && (line[n_read - 1] == '\n' || line[n_read - 1] == ' ' || line[n_read - 1] == '\t'))
        line[--n_read] = '\0';

    if(n_read == 0) {
        char *empty = strdup("");
        free(line);
        return empty;
    }

    return line;
}

char **Input_Parser(char *user_input) {
    char **command_list = malloc(200 * sizeof(char *));
    int i = 0;
    char *list_elts;
    char *tmp = strdup(user_input);
    char *ptr = tmp;

    list_elts = strsep(&ptr, " ");
    while(list_elts != NULL) {
        if (strlen(list_elts) > 0) {
            command_list[i] = strdup(list_elts);
            i++;
        }
        list_elts = strsep(&ptr, " ");
    }
    command_list[i] = NULL;
    free(tmp);
    return command_list;
}

void Sig_Child(int sig) {
    int err_no_save = errno;
    pid_t pid;
    int status;
    struct timeval t_end;

    while((pid = waitpid(-1, &status, WNOHANG)) > 0) {

        gettimeofday(&t_end, NULL);
        for(int i = 0; i < history_counter; i++) {

            if(history[i].pid == pid && history[i].t_finish <= 0.0) {
                double t_finish = (t_end.tv_sec - history[i].t_start.tv_sec) +
                                  (t_end.tv_usec - history[i].t_start.tv_usec) / 1e6;
                history[i].t_finish = t_finish;
                break;
            }
        }
    }
    errno = err_no_save;
}

void print_final_report(Command_History history[], int count, int TSLICE) {
    printf("\nFinal Report:\n");
    printf("Name\tPID\tCompletion_time(s)\tWait_time(s)\n");

    double total_comp = 0, total_wait = 0;

    for(int i = 0; i < count; i++) {
        int completion_slices = (int)((history[i].t_finish * 1000 + TSLICE - 1) / TSLICE);
        if(completion_slices < 1) completion_slices = 1;
        int waiting_slices = (completion_slices > 0) ? completion_slices - 1 : 0;

        double completion_sec = (completion_slices * TSLICE) / 1000.0;
        double waiting_sec = (waiting_slices * TSLICE) / 1000.0;

        printf("%s\t%d\t%.2f\t%.2f\n",
               history[i].command,
               history[i].pid,
               completion_sec,
               waiting_sec);

        total_comp+= completion_sec;
        total_wait+= waiting_sec;
    }

    if (count > 0) {
        printf("average completion time: %.2f sec\n", total_comp / count);
        printf("average waiting Time: %.2f sec\n", total_wait / count);
    }
}

void Sig_Int(int sig) {
    printf("\nCtrl+C entered Scheduler terminating\n");

    if (sch_pid > 0)
        kill(sch_pid, SIGTERM);

    if (sch_fd >= 0)
        close(sch_fd);

    if (sch_pid > 0)
        waitpid(sch_pid, NULL, 0);

    print_final_report(history, history_counter, TSLICE);
    printf("Exiting\n");
    exit(0);
}

void Run_Command(char **cmd_list, char *input) {
    printf("running command: %s\n", input);
    if (history_counter >= 400) return;

    pid_t pid;
    struct timeval t_start, t_end;
    gettimeofday(&t_start, NULL);

    pid = fork();
    if(pid == 0) {
        close(sch_fd);
        execvp(cmd_list[0], cmd_list);
        printf("Execution failed");
        exit(1);
    } 
    else if(pid > 0) {
        wait(NULL);
        gettimeofday(&t_end, NULL);

        double t_finish = (t_end.tv_sec - t_start.tv_sec) +
                          (t_end.tv_usec - t_start.tv_usec) / 1e6;

        strcpy(history[history_counter].command, input);
        history[history_counter].t_start = t_start;
        history[history_counter].t_finish = t_finish;
        history[history_counter].pid = pid;
        history_counter++;

        printf("Command '%s' finished in %.4f sec\n", input, t_finish);
    } 
    else {
        printf("Fork failed");
    }
}

int Launch(char *input) {
    if(strncmp(input, "submit", 6) == 0) {
        char prog[128];
        if (sscanf(input, "submit %127s", prog) != 1) {
            printf("Usage: submit ./program\n");
            return 1;
        }

        printf("Submitting job: %s\n", prog);
        pid_t pid = fork();

        if(pid == 0) {
            close(sch_fd);
            execl(prog, prog, NULL);
            printf("Exec failed");
            _exit(1);
        }

        if(history_counter < 400) {

            gettimeofday(&history[history_counter].t_start, NULL);
            strncpy(history[history_counter].command, input, sizeof(history[history_counter].command) - 1);
            history[history_counter].command[sizeof(history[history_counter].command) - 1] = '\0';
            history[history_counter].pid = pid;
            history[history_counter].t_finish = 0.0;
            history_counter++;
        }

        int status;
        waitpid(pid, &status, WUNTRACED);

        if(WIFSTOPPED(status)) {
            //printf("Job '%s' stopped. Sending PID=%d to scheduler.\n", prog, pid);
            if (dprintf(sch_fd, "%d %s\n", pid, prog) < 0)
                perror("failed to send job to scheduler");
            fsync(sch_fd);
        } 
        else{
            printf("job '%s' not stopped \n", prog);
        }
        return 1;
    }

    if(strcmp(input, "history") == 0) {
        for (int i = 0; i < history_counter; i++)
            printf("%s\n", history[i].command);
        return 1;
    }

    if (strcmp(input,"time") == 0) {
        time_t t = time(NULL);
        printf("current time: %s", ctime(&t));
        return 1;
    }

    char *temp = strdup(input);
    char *com;
    char *parts[50];
    int n= 0;

    while((com = strsep(&temp, "|")) != NULL) {
        parts[n] = strdup(com);
        n++;
    }
    free(temp);

    if(n > 1) {
        for (int i = 0; i < n; i++) free(parts[i]);
    } 
    else{
        char **cmd = Input_Parser(input);
        Run_Command(cmd, input);
        for (int i = 0; cmd[i] != NULL; ++i) free(cmd[i]);
        free(cmd);
    }

    for(int i = 0; i < n; i++) free(parts[i]);
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s NCPU TSLICE\n", argv[0]);
        exit(1);
    }

    NCPU = atoi(argv[1]);
    TSLICE = atoi(argv[2]);
    printf("NCPU=%d , TSLICE=%dms\n",NCPU,TSLICE);

    int pipefd[2];
    if (pipe(pipefd) < 0) {
        printf("pipe creation failed eror");
        exit(1);
    }

    sch_pid = fork();
    if (sch_pid == 0) {
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        execl("./SimpleScheduler","./SimpleScheduler",argv[1],argv[2],NULL);
        perror("Failed to start scheduler");
        _exit(1);
    }

    close(pipefd[0]);
    sch_fd = pipefd[1];
    fcntl(sch_fd,F_SETFL,O_NONBLOCK);

    printf("scheduler started with PID=%d\n", sch_pid);

    signal(SIGINT, Sig_Int);
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = Sig_Child;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);

    fd_set readfds;
    int stdin_fd = fileno(stdin);

    while (1) {
        printf("SimpleShell$ ");
        fflush(stdout);

        FD_ZERO(&readfds);
        FD_SET(stdin_fd, &readfds);

        int s = select(stdin_fd + 1, &readfds, NULL, NULL, NULL);
        if (s < 0) {
            if (errno == EINTR) continue;
            perror("Select error");
            break;
        }

        if (FD_ISSET(stdin_fd, &readfds)) {
            char *input = Input();
            if (input == NULL) {
                printf("Shell exiting.\n");
                close(sch_fd);
                if (sch_pid > 0) kill(sch_pid, SIGTERM);
                if (sch_pid > 0) waitpid(sch_pid, NULL, 0);
                print_final_report(history, history_counter, TSLICE);
                break;
            }

            if (strcmp(input, "exit") == 0) {
                printf("Exiting shell.\n");
                close(sch_fd);
                if (sch_pid > 0) kill(sch_pid, SIGTERM);
                if (sch_pid > 0) waitpid(sch_pid, NULL, 0);
                print_final_report(history, history_counter, TSLICE);
                free(input);
                break;
            }

            if (strlen(input) > 0)
                Launch(input);

            free(input);
        }
    }

    return 0;
}
