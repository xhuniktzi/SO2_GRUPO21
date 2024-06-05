#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <signal.h>

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
    fdch = open("practica1.txt", O_RDWR, 0777);
    signal(SIGINT, ctrlc_handler);

    while (!sigint_received) {
        int time = rand() % 3 + 1;
        int op = rand() % 3 + 1;

        if (op == 1) {
            char random_string[8];
            for (int i = 0; i < 8; ++i) {
                random_string[i] = random_char();
            }
            write(fdch, random_string, 8);
        } else if (op == 2) {
            char buff[8];
            read(fdch, buff, 8);
        } else {
            lseek(fdch, 0, SEEK_SET);
        }
        sleep(time);
    }
    return 0;
}
