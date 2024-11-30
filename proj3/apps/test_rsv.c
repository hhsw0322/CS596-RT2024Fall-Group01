#include <stdio.h>
#include <sys/syscall.h>
#include <time.h>
#include <sched.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

void budget_overrun_handler(int sig) {
    printf("Received budget overrun signal (SIGUSR1)\n");
}

int main() {
    struct timespec C = {1, 0}; // Reservation budget of 1 second
    struct timespec T = {3, 0}; // Reservation period of 3 seconds

    // Register signal handler for SIGUSR1
    struct sigaction sa;
    sa.sa_handler = budget_overrun_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction failed");
        return 1;
    }

    // Set reservation for the current process
    if (syscall(397, 0, &C, &T) == -1) {
        perror("set_rsv failed");
        return 1;
    }
    printf("set_rsv successful\n");

    // Perform periodic waiting
    for (int i = 0; i < 5; i++) {
        // Do some computation to simulate workload
        for (volatile int j = 0; j < 100000000; j++);
        printf("Completed computation in iteration %d\n", i + 1);

        // Wait until the next period
        if (syscall(399) == -1) {
            perror("wait_until_next_period failed");
            return 1;
        }
        printf("Woke up for next period\n");
    }

    // Cancel the reservation
    if (syscall(398, 0) == -1) {
        perror("cancel_rsv failed");
        return 1;
    }
    printf("cancel_rsv successful\n");

    printf("To verify, use: dmesg | grep 'Reservation'\n");

    return 0;
}
