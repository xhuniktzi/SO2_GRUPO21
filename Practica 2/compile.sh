#! /bin/bash

gcc -o bank_operations.bin bank_operations.c -lpthread -ljansson
chmod +x bank_operations.bin