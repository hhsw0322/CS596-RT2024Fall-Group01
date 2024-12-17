#define _GNU_SOURCE
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>

#define SET_RSV_SYSCALL 397
#define WAIT_NEXT_PERIOD_SYSCALL 399
#define CANCEL_RSV_SYSCALL 398

#define BUTTON_GPIO_PIN 26 // GPIO Pin for the button
#define GPIO_PATH "/sys/class/gpio"
volatile int button_pressed = 0;

void button_monitor_task();
void logging_task();
void sigusr1_handler(int sig) {
    printf("Task budget overrun! Signal: %d\n", sig);
}

// Export GPIO pin
void export_gpio(int pin) {
    int fd = open(GPIO_PATH "/export", O_WRONLY);
    if (fd < 0) {
        perror("Failed to export GPIO");
        exit(1);
    }
    dprintf(fd, "%d", pin);
    close(fd);
}

// Set GPIO direction
void set_gpio_direction(int pin, const char *direction) {
    char path[50];
    snprintf(path, sizeof(path), GPIO_PATH "/gpio%d/direction", pin);
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        perror("Failed to set GPIO direction");
        exit(1);
    }
    write(fd, direction, strlen(direction));
    close(fd);
}

// Read GPIO value
int read_gpio_value(int pin) {
    char path[50];
    char value;
    snprintf(path, sizeof(path), GPIO_PATH "/gpio%d/value", pin);
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("Failed to read GPIO value");
        return -1;
    }
    read(fd, &value, 1);
    close(fd);
    return (value == '1') ? 1 : 0;
}

int main() {
    pid_t button_task_pid, logging_task_pid;
    struct timespec C = {1, 0}; // Budget: 1 second
    struct timespec T = {2, 0}; // Period: 2 seconds

    signal(SIGUSR1, sigusr1_handler); // Register signal handler

    // Initialize GPIO
    export_gpio(BUTTON_GPIO_PIN);
    set_gpio_direction(BUTTON_GPIO_PIN, "in");

    // Fork the first task (button monitor)
    button_task_pid = fork();
    if (button_task_pid == 0) {
        printf("Task 1: Button monitoring started...\n");
        if (syscall(SET_RSV_SYSCALL, 0, &C, &T) == -1) {
            perror("Task 1: set_rsv failed");
            exit(1);
        }
        button_monitor_task();
        syscall(CANCEL_RSV_SYSCALL, 0);
        exit(0);
    }

    // Fork the second task (logging task)
    logging_task_pid = fork();
    if (logging_task_pid == 0) {
        printf("Task 2: Logging task started...\n");
        if (syscall(SET_RSV_SYSCALL, 0, &C, &T) == -1) {
            perror("Task 2: set_rsv failed");
            exit(1);
        }
        logging_task();
        syscall(CANCEL_RSV_SYSCALL, 0);
        exit(0);
    }

    // Wait for both tasks to finish
    wait(NULL);
    wait(NULL);
    printf("Both tasks completed successfully.\n");
    return 0;
}

void button_monitor_task() {
    while (1) {
        int button_state = read_gpio_value(BUTTON_GPIO_PIN);
        if (button_state == 1) {
            printf("Task 1: Button Pressed!\n");
            button_pressed = 1;
        } else {
            button_pressed = 0;
        }

        if (syscall(WAIT_NEXT_PERIOD_SYSCALL) == -1) {
            perror("Task 1: wait_until_next_period failed");
            break;
        }
    }
}

void logging_task() {
    while (1) {
        printf("Task 2: Periodic logging task executed at %ld seconds\n", time(NULL));
        if (syscall(WAIT_NEXT_PERIOD_SYSCALL) == -1) {
            perror("Task 2: wait_until_next_period failed");
            break;
        }
    }
}
