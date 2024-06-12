#include "headers.h"

pthread_mutex_t lock;

User *user_list = NULL;
char user_errors[MAX_ERROR_LENGTH] = "";
char operation_errors[MAX_ERROR_LENGTH] = "";
int total_users_loaded = 0;
int users_per_thread[THREADS_USERS] = {0};
int total_operations_loaded = 0;
int operations_per_thread[THREADS_OPERATIONS] = {0};
