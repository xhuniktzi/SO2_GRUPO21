#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>

// Declaraciones de funciones
void handle_sigint(int sig);
void log_syscall(const char *syscall_info);
void monitor_syscalls(int pid1, int pid2);
void stat(int pid1, int pid2, int monitor, int fd);

int syscall_count = 0;
int read_count = 0;
int write_count = 0;
int open_count = 0;
int pipe_fd[2];

volatile sig_atomic_t sigint_received = 0; // Variable compartida para indicar si se recibió la señal SIGINT

void handle_sigint(int signal) {
    sigint_received = 1; // Establecer la bandera al recibir la señal SIGINT
}

void stat(int pid1, int pid2, int monitor, int fd) {
    printf("\n");
    printf("Finalizando ejecución...\n");
    char buff[1024];
    char *action;
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buff, sizeof(buff))) > 0) {
        // Busca las acciones en el buffer
        char *ptr = buff;
        while ((action = strtok(ptr, "\n")) != NULL) {
            ptr = NULL; // Siguiente llamada a strtok debe recibir NULL
            // Incrementa el contador según la acción
            if (strstr(action, "read") != NULL) {
                read_count++;
            } else if (strstr(action, "lseek") != NULL) {
                open_count++;
            } else if (strstr(action, "write") != NULL) {
                write_count++;
            }
        }
    }
    syscall_count = read_count + open_count + write_count;

    printf("-------------------------------------------------\n");
    printf("Llamadas al sistema de los procesos hijo: %d      \n", syscall_count);
    printf("Llamadas write: %d                                \n", write_count);
    printf("Llamadas read: %d                                 \n", read_count);
    printf("Llamadas seek %d                                  \n", open_count);
    printf("-------------------------------------------------\n");
    close(fd);
    kill(pid1, SIGKILL);
    kill(pid2, SIGKILL);
    kill(monitor, SIGKILL);
    exit(0);
}

void log_syscall(const char *syscall_info) {
    FILE *log = fopen("syscalls.log", "a");
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

void monitor_syscalls(int pid1, int pid2) {
    char command[200];
    sprintf(command, "stap -v -e 'global pid1=%d, pid2=%d; probe syscall.read { if (pid() == pid1 || pid() == pid2) { printf(\"%%d:read(%%s)\\n\", pid(), ctime(gettimeofday_s())) } } probe syscall.write { if (pid() == pid1 || pid() == pid2) { printf(\"%%d:write(%%s)\\n\", pid(), ctime(gettimeofday_s())) } } probe syscall.lseek { if (pid() == pid1 || pid() == pid2) { printf(\"%%d:lseek(%%s)\\n\", pid(), ctime(gettimeofday_s())) } }' > syscalls.log", pid1, pid2);
    system(command);
}

int main() {
    signal(SIGINT, handle_sigint);
    FILE *log = fopen("syscalls.log", "w");
    if (log == NULL) {
        perror("Failed to open log file");
        return 1;
    }
    fclose(log);

    int fdch = open("practica1.txt", O_CREAT | O_TRUNC, 0777);
    close(fdch);

    if (pipe(pipe_fd) == -1) {
        perror("pipe failed");
        return 1;
    }

    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("fork failed");
        return 1;
    }

    if (pid1 == 0) {
        execl("./child_proc.bin", "./child_proc.bin", NULL);
        perror("execl failed");
        return 1;
    } else {
        pid_t pid2 = fork();
        if (pid2 == -1) {
            perror("fork failed");
            return 1;
        }

        if (pid2 == 0) {
            execl("./child_proc.bin", "./child_proc.bin", NULL);
            perror("execl failed");
            return 1;
        } else {
            pid_t monitor = fork();
            if (monitor == 0) {
                monitor_syscalls(pid1, pid2);
            }

            int status;
            while (!sigint_received) {
                sleep(1);
            }
            stat(pid1, pid2, monitor, open("syscalls.log", O_RDONLY));
        }
    }
    return 0;
}
