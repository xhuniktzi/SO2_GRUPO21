#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <signal.h>

#define DATA_FILE "practica1.txt"
#define BUFFER_SIZE 8

volatile sig_atomic_t sigint_received = 0;
int fdch;

void ctrlc_handler(int signal) {
    sigint_received = 1;
    close(fdch);
}

char random_char() {
    const char charset[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const int charset_size = sizeof(charset) - 1;
    return charset[rand() % charset_size];
}

int main(int argc, char *argv[]) {
    srand(time(NULL) + getpid());
    fdch = open(DATA_FILE, O_RDWR, 0777);
    signal(SIGINT, ctrlc_handler);

    while (!sigint_received) {
        int time = rand() % 3 + 1;
        int op = rand() % 3 + 1;

        if (op == 1) {
            char random_string[BUFFER_SIZE];
            for (int i = 0; i < BUFFER_SIZE; ++i) {
                random_string[i] = random_char();
            }
            write(fdch, random_string, BUFFER_SIZE);
        } else if (op == 2) {
            char buff[BUFFER_SIZE];
            read(fdch, buff, BUFFER_SIZE);
        } else {
            lseek(fdch, 0, SEEK_SET);
        }
        sleep(time);
    }
    return 0;
}
