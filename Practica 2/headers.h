#ifndef HEADERS_H
#define HEADERS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include <jansson.h>

#define THREADS_USERS 3
#define THREADS_OPERATIONS 4
#define MAX_USERS 1000
#define MAX_OPERATIONS 1000
#define MAX_ERROR_LENGTH 10000
#define MAX_NAME_LENGTH 100

extern pthread_mutex_t lock;

typedef struct User
{
    int account_number;
    char name[MAX_NAME_LENGTH];
    float balance;
    struct User *next;
} User;

typedef struct Operation
{
    int type; // 1 -> Deposit, 2 -> Withdrawal, 3 -> Transfer
    int account1;
    int account2;
    float amount;
} Operation;

typedef struct ThreadInfo
{
    int id;
    json_t *root;
    int start_index;
    int end_index;
} ThreadInfo;

extern User *user_list;
extern char user_errors[MAX_ERROR_LENGTH];
extern char operation_errors[MAX_ERROR_LENGTH];
extern int total_users_loaded;
extern int users_per_thread[THREADS_USERS];
extern int total_operations_loaded;
extern int operations_per_thread[THREADS_OPERATIONS];

void insert_user(int account_number, const char *name, float balance);
bool account_exists(int account_number);
User *get_user(int account_number);
void deposit(int account_number, float amount);
void withdraw(int account_number, float amount, int line_number);
void transfer(int account1, int account2, float amount, int line_number);
void *load_users(void *arg);
void *load_operations(void *arg);
void generate_user_report();
void generate_operation_report();
void generate_balance_report();
void load_users_multithreaded(const char *filename);
void load_operations_multithreaded(const char *filename);

#endif
