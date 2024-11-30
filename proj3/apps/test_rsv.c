#include <stdio.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

int main() {
    struct timespec C = {1, 0}; // Reservation budget of 1 second
    struct timespec T = {5, 0}; // Reservation period of 5 seconds

    // Set reservation for the current process
    if (syscall(397, 0, &C, &T) == -1) {
        perror("set_rsv failed");
    } else {
        printf("set_rsv successful\n");
    }

    // Cancel reservation for the current process
    if (syscall(398, 0) == -1) {
        perror("cancel_rsv failed");
    } else {
        printf("cancel_rsv successful\n");
    }

    // Remind user to check kernel logs for verification
    printf("To verify, use: dmesg | grep 'Reservation'\n");

    return 0;
}
