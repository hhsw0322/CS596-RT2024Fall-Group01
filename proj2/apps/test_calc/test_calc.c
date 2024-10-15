#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdlib.h>

#define SYS_calc 397  // System call number

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <int1> <operation> <int2>\n", argv[0]);
        return -1;
    }

    int param1 = atoi(argv[1]);
    char operation = argv[2][0];
    int param2 = atoi(argv[3]);
    int result;

    // Make the system call
    long ret = syscall(SYS_calc, param1, param2, operation, &result);
    if (ret == -1) {
        printf("Error: invalid operation or input\n");
    } else {
        printf("Result: %d\n", result);
    }

    return 0;
}
