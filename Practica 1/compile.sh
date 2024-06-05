#! /bin/bash

gcc -o parent_proc.bin parent_proc.c
gcc -o child_proc.bin child_proc.c
chmod +x parent_proc.bin child_proc.bin