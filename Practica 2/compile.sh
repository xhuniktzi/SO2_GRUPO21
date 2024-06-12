#! /bin/bash

gcc -o bank_operations.bin main.c user_operations.c file_operations.c globals.c -lpthread -ljansson
chmod +x bank_operations.bin