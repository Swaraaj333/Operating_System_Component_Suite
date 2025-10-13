#include <stdio.h>
#include <unistd.h>
#include "dummy_main.h"

long long fib(long long n) {
    if (n <= 1)
        return n;
    return fib(n - 1) + fib(n - 2);
}

int main(int argc, char **argv) {
    printf("[fib] PID %d started", getpid());
    fflush(stdout);

    long long result = fib(46);

    printf("[fib] PID %d finished. Fibonacci(46) = %lld\n", getpid(), result);
    fflush(stdout);

    return 0;
}