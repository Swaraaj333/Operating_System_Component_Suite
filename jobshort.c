#include <stdio.h>
#include <unistd.h>
#include "dummy_main.h"

int main(int argc, char **argv) {
    printf("short job PID %d runs\n", getpid());
    volatile unsigned long long i;
    for (i = 0; i < 1000000000ULL; i++);
    printf("short job PID %d finishes\n", getpid());
    return 0;
}