#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>

#define LOG_FILE "syscalls.log"

int syscall_count = 0;
int read_count = 0;
int write_count = 0;
int open_count = 0;
int pipe_fd[2];

void handle_sigint(int sig) {
    char buffer[256];
    int bytes_read;

    // Leer lo que queda en el pipe
    while ((bytes_read = read(pipe_fd[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        log_syscall(buffer);
    }

    printf("Total system calls: %d\n", syscall_count);
    printf("Read: %d, Write: %d, Open: %d\n", read_count, write_count, open_count);
    exit(0);
}

void log_syscall(const char *syscall_info) {
    FILE *log = fopen(LOG_FILE, "a");
    if (log == NULL) {
        perror("Failed to open log file");
        exit(1);
    }

    char syscall[16];
    int pid;
    sscanf(syscall_info, "Process %d: %s", &pid, syscall);

    fprintf(log, "%s\n", syscall_info);
    fclose(log);

    syscall_count++;
    if (strcmp(syscall, "read") == 0) read_count++;
    if (strcmp(syscall, "write") == 0) write_count++;
    if (strcmp(syscall, "open") == 0) open_count++;
}

void monitor_syscalls() {
    char buffer[256];
    int bytes_read;

    while ((bytes_read = read(pipe_fd[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        log_syscall(buffer);
    }
}

int main() {
    signal(SIGINT, handle_sigint);
    FILE *log = fopen(LOG_FILE, "w");
    if (log == NULL) {
        perror("Failed to open log file");
        return 1;
    }
    fclose(log);

    if (pipe(pipe_fd) == -1) {
        perror("pipe failed");
        return 1;
    }

    pid_t stap_pid = fork();
    if (stap_pid == -1) {
        perror("fork failed");
        return 1;
    }

    if (stap_pid == 0) {
        // Redirigir la salida estándar a la entrada del pipe
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[0]);
        close(pipe_fd[1]);

        execl("/usr/bin/stap", "stap", "-v", "monitor.stp", NULL);
        perror("execl failed");
        return 1;
    } else {
        close(pipe_fd[1]);

        pid_t child1 = fork();
        if (child1 == 0) {
            execl("./child_proc.bin", "./child_proc.bin", NULL);
            perror("execl failed");
            return 1;
        }

        pid_t child2 = fork();
        if (child2 == 0) {
            execl("./child_proc.bin", "./child_proc.bin", NULL);
            perror("execl failed");
            return 1;
        }

        monitor_syscalls();

        // Esperar a que terminen los procesos hijo
        wait(NULL);
        wait(NULL);
        wait(NULL);
    }

    return 0;
}
