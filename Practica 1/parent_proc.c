#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>

// Constantes
#define LOG_FILE "syscalls.log"
#define DATA_FILE "practica1.txt"
#define PIPE_READ 0
#define PIPE_WRITE 1

// Declaraciones de funciones
void handle_sigint(int sig);
void log_syscall(const char *syscall_info);
void monitor_syscalls(int pid1, int pid2);
void finalize_stats(int pid1, int pid2, int monitor, int fd);
void update_stats(const char *buffer);

// Estructura para contar las llamadas a sistema
typedef struct {
    int syscall_count;
    int read_count;
    int write_count;
    int open_count;
} syscall_stats_t;

syscall_stats_t stats = {0, 0, 0, 0};
int pipe_fd[2];
volatile sig_atomic_t sigint_received = 0; // Variable compartida para indicar si se recibió la señal SIGINT

void handle_sigint(int signal) {
    sigint_received = 1; // Establecer la bandera al recibir la señal SIGINT
}

void update_stats(const char *buffer) {
    char action[16];
    const char *ptr = buffer;
    
    while (sscanf(ptr, "%15s", action) == 1) {
        if (strcmp(action, "read") == 0) {
            stats.read_count++;
        } else if (strcmp(action, "lseek") == 0) {
            stats.open_count++;
        } else if (strcmp(action, "write") == 0) {
            stats.write_count++;
        }
        stats.syscall_count = stats.read_count + stats.open_count + stats.write_count;
        ptr += strlen(action) + 1;
    }
}

void finalize_stats(int pid1, int pid2, int monitor, int fd) {
    printf("\nFinalizando ejecución...\n");
    char buff[1024];
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buff, sizeof(buff))) > 0) {
        update_stats(buff);
    }

    printf("Llamadas totales:\t%d\n", stats.syscall_count);
    printf("Llamadas write:\t\t%d\n", stats.write_count);
    printf("Llamadas read:\t\t%d\n", stats.read_count);
    printf("Llamadas seek:\t\t%d\n", stats.open_count);
    close(fd);
    kill(pid1, SIGKILL);
    kill(pid2, SIGKILL);
    kill(monitor, SIGKILL);
    exit(0);
}

void log_syscall(const char *syscall_info) {
    FILE *log = fopen(LOG_FILE, "a");
    if (log == NULL) {
        perror("Failed to open log file");
        exit(1);
    }

    fprintf(log, "%s\n", syscall_info);
    fclose(log);
}

void monitor_syscalls(int pid1, int pid2) {
    char command[512];
    snprintf(command, sizeof(command), "stap monitor.stp %d %d > " LOG_FILE,
             pid1, pid2);
    system(command);
}

int main() {
    signal(SIGINT, handle_sigint);

    FILE *log = fopen(LOG_FILE, "w");
    if (log == NULL) {
        perror("Failed to open log file");
        return 1;
    }
    fclose(log);

    int fdch = open(DATA_FILE, O_CREAT | O_TRUNC, 0777);
    close(fdch);

    if (pipe(pipe_fd) == -1) {
        perror("pipe failed");
        return 1;
    }

    pid_t pid1 = fork();
    switch (pid1) {
        case -1:
            perror("fork failed");
            return 1;
        case 0:
            execl("./child_proc.bin", "./child_proc.bin", NULL);
            perror("execl failed");
            return 1;
        default:
            break;
    }

    pid_t pid2 = fork();
    switch (pid2) {
        case -1:
            perror("fork failed");
            return 1;
        case 0:
            execl("./child_proc.bin", "./child_proc.bin", NULL);
            perror("execl failed");
            return 1;
        default:
            break;
    }

    pid_t monitor = fork();
    switch (monitor) {
        case -1:
            perror("fork failed");
            return 1;
        case 0:
            monitor_syscalls(pid1, pid2);
            break;
        default:
            while (!sigint_received) {
                pause();
            }
            finalize_stats(pid1, pid2, monitor, open(LOG_FILE, O_RDONLY));
            break;
    }
    return 0;
}
