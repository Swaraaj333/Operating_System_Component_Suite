#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/time.h>  
#include <sys/select.h>
#include <sys/types.h>

#define J_MAX 100

typedef struct {
    pid_t pid;
    char name[256];
    int completed;
    int slice_run;
    struct timeval t_arrival;
    struct timeval t_start;
    struct timeval t_end;
    int started;
} Job;

Job Ready_Queue[J_MAX];
int front = 0, rear = 0;
int ncpu, t_slice;


double time_diff_ms(struct timeval start, struct timeval end) {
    return (end.tv_sec - start.tv_sec) * 1000.0 +
           (end.tv_usec - start.tv_usec) / 1000.0;
}

int check_empty() {
    return front == rear;
}

void enqueue(pid_t pid, const char *name) {
    if ((rear + 1) % J_MAX == front) {
        return;
    }

    Ready_Queue[rear].pid = pid;
    strncpy(Ready_Queue[rear].name, name, sizeof(Ready_Queue[rear].name) - 1);
    Ready_Queue[rear].name[sizeof(Ready_Queue[rear].name) - 1] = '\0';
    Ready_Queue[rear].completed = 0;
    Ready_Queue[rear].slice_run = 0;
    Ready_Queue[rear].started = 0;
    gettimeofday(&Ready_Queue[rear].t_arrival, NULL); // arrival time

    rear = (rear + 1) % J_MAX;
    printf("job has enqueued in the queue: PID=%d (%s)\n", pid, name);
    fflush(stdout);
}

Job dequeue() {
    Job j = Ready_Queue[front];
    front = (front + 1) % J_MAX;
    return j;
}

void remove_completed() {
    Job tmp[J_MAX];
    int new_rear = 0;

    for (int i = front; i != rear; i = (i + 1) % J_MAX) {
        if (!Ready_Queue[i].completed) {
            tmp[new_rear++] = Ready_Queue[i];
        }
    }

    front = 0;
    rear = new_rear;
    for (int i = 0; i < new_rear; i++) {
        Ready_Queue[i] = tmp[i];
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Scheduler info : %s NCPU TSLICE( in ms) \n", argv[0]);
        exit(1);
    }

    ncpu = atoi(argv[1]);
    t_slice = atoi(argv[2]);
    printf("NCPU=%d , TSLICE=%dms\n", ncpu, t_slice);
    fflush(stdout);

    int pipe_fd = fileno(stdin);
    fd_set readfds;
    double total_turnaround = 0.0;
    int completed_jobs = 0;

    while (1) {
        int status;
        while (waitpid(-1, &status, WNOHANG) > 0);

        if (check_empty()) {
            FD_ZERO(&readfds);
            FD_SET(pipe_fd, &readfds);
            fflush(stdout);

            int ret = select(pipe_fd + 1, &readfds, NULL, NULL, NULL);

            if (ret > 0 && FD_ISSET(pipe_fd, &readfds)) {
                char buf[512];
                if (fgets(buf, sizeof(buf), stdin)) {
                    pid_t jobpid;
                    char jobname[256];

                    if (sscanf(buf, "%d %255s", &jobpid, jobname) == 2) {
                        enqueue(jobpid, jobname);
                    }
                } else if (feof(stdin)) {
                    break;
                }
            }
        }

        if (!check_empty()) {
            int jobs_this_cycle = 0;
            pid_t running_pids[ncpu];
            char running_names[ncpu][256];

            for (int i = 0; i < ncpu && !check_empty(); i++) {
                Job job = dequeue();

                if (!job.started) {
                    gettimeofday(&job.t_start, NULL);
                    job.started = 1;
                }

                printf("scheduler starts PID %d (%s)\n", job.pid, job.name);
                fflush(stdout);

                if (kill(job.pid, SIGCONT) != 0) {
                    continue;
                }

                running_pids[jobs_this_cycle] = job.pid;
                strncpy(running_names[jobs_this_cycle], job.name, sizeof(running_names[0]));
                running_names[jobs_this_cycle][sizeof(running_names[0]) - 1] = '\0';
                jobs_this_cycle++;
                job.slice_run++;
            }

            usleep(t_slice * 1000);

            for (int i = 0; i < jobs_this_cycle; i++) {
                pid_t pid = running_pids[i];
                int status;
                pid_t ret = waitpid(pid, &status, WNOHANG);

                if (ret == 0) {
                    enqueue(pid, running_names[i]);
                } else if (ret == pid) {
                    struct timeval end;
                    gettimeofday(&end, NULL);

                    for (int j = 0; j < J_MAX; j++) {
                        if (Ready_Queue[j].pid == pid) {
                            Ready_Queue[j].completed = 1;
                            Ready_Queue[j].t_end = end;
                            // this i have handled in shell teport 
                            double turnaround = time_diff_ms(Ready_Queue[j].t_arrival, end);
                            double wait = turnaround - (Ready_Queue[j].slice_run * t_slice);

                            printf("PID %d has completed.\n", pid);
                            printf("Turnaround Time (T_complete): %.2f ms\n", turnaround);
                            printf("Waiting Time (T_wait): %.2f ms\n", wait);
                            fflush(stdout);

                            total_turnaround += turnaround;
                            completed_jobs++;
                            break;
                        }
                    }
                }
            }

            remove_completed();

            if (check_empty()) {
                int alive = 0;
                for (int i = 0; i < J_MAX; i++) {
                    if (Ready_Queue[i].pid > 0 && kill(Ready_Queue[i].pid, 0) == 0) {
                        alive = 1;
                        break;
                    }
                }
                if (!alive) {
                    if (completed_jobs > 0) {
                        double avg_completion = total_turnaround / completed_jobs;
                        printf("\nAverage Completion Time: %.2f ms\n", avg_completion);
                    }
                    break;
                }
            }
        }
    }

    printf("Scheduler exiting.\n");
    fflush(stdout);
    return 0;
}
