#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>

#define FILENAME "practica1.txt"

void random_string(char *str, size_t size) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (size_t i = 0; i < size; i++) {
        int key = rand() % (int)(sizeof(charset) - 1);
        str[i] = charset[key];
    }
    str[size] = '\0';
}

void random_sleep() {
    int sleep_time = (rand() % 3 + 1) * 1000000; // 1 to 3 seconds in microseconds
    usleep(sleep_time);
}

int main() {
    srand(time(NULL));
    int fd = open(FILENAME, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd == -1) {
        perror("Failed to open file");
        return 1;
    }

    while (1) {
        int action = rand() % 3;
        char buffer[9];
        switch (action) {
            case 0: // Read
                lseek(fd, 0, SEEK_SET);
                read(fd, buffer, 8);
                buffer[8] = '\0';
                break;
            case 1: // Write
                random_string(buffer, 8);
                write(fd, buffer, 8);
                break;
            case 2: // Open (simulated by reopening the file)
                close(fd);
                fd = open(FILENAME, O_RDWR);
                break;
        }
        random_sleep();
    }

    close(fd);
    return 0;
}
