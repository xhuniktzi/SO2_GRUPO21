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

pthread_mutex_t lock;

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

User *user_list = NULL;
char user_errors[MAX_ERROR_LENGTH] = "";
char operation_errors[MAX_ERROR_LENGTH] = "";
int total_users_loaded = 0;
int users_per_thread[THREADS_USERS] = {0};
int total_operations_loaded = 0;
int operations_per_thread[THREADS_OPERATIONS] = {0};

void insert_user(int account_number, const char *name, float balance)
{
    User *new_user = (User *)malloc(sizeof(User));
    new_user->account_number = account_number;
    strcpy(new_user->name, name);
    new_user->balance = balance;
    new_user->next = user_list;
    user_list = new_user;
}

bool account_exists(int account_number)
{
    User *current = user_list;
    while (current != NULL)
    {
        if (current->account_number == account_number)
        {
            return true;
        }
        current = current->next;
    }
    return false;
}

User *get_user(int account_number)
{
    User *current = user_list;
    while (current != NULL)
    {
        if (current->account_number == account_number)
        {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void deposit(int account_number, float amount)
{
    User *user = get_user(account_number);
    if (user)
    {
        user->balance += amount;
    }
}

void withdraw(int account_number, float amount, int line_number)
{
    User *user = get_user(account_number);
    if (user && user->balance >= amount)
    {
        user->balance -= amount;
    }
    else
    {
        if (line_number > 0)
        {
            char error[200];
            snprintf(error, sizeof(error), "Line #%d - Insufficient balance in account %d\n", line_number, account_number);
            strncat(operation_errors, error, sizeof(operation_errors) - strlen(operation_errors) - 1);
        }
    }
}

void transfer(int account1, int account2, float amount, int line_number)
{
    User *user1 = get_user(account1);
    User *user2 = get_user(account2);
    if (!user1)
    {
        char error[200];
        snprintf(error, sizeof(error), "Line #%d - Source account %d does not exist\n", line_number, account1);
        strncat(operation_errors, error, sizeof(operation_errors) - strlen(operation_errors) - 1);
        return;
    }
    if (!user2)
    {
        char error[200];
        snprintf(error, sizeof(error), "Line #%d - Destination account %d does not exist\n", line_number, account2);
        strncat(operation_errors, error, sizeof(operation_errors) - strlen(operation_errors) - 1);
        return;
    }
    if (user1->balance < amount)
    {
        char error[200];
        snprintf(error, sizeof(error), "Line #%d - Insufficient balance in account %d\n", line_number, account1);
        strncat(operation_errors, error, sizeof(operation_errors) - strlen(operation_errors) - 1);
        return;
    }
    user1->balance -= amount;
    user2->balance += amount;
}

void *load_users(void *arg)
{
    ThreadInfo *info = (ThreadInfo *)arg;

    for (int i = info->start_index; i < info->end_index; i++)
    {
        json_t *value = json_array_get(info->root, i);

        int account = json_integer_value(json_object_get(value, "no_cuenta"));
        const char *name = json_string_value(json_object_get(value, "nombre"));
        float balance = json_real_value(json_object_get(value, "saldo"));

        pthread_mutex_lock(&lock);
        if (account_exists(account))
        {
            char error[200];
            snprintf(error, sizeof(error), "Line #%d - Duplicate account %d\n", i + 1, account);
            strncat(user_errors, error, sizeof(user_errors) - strlen(user_errors) - 1);
        }
        else if (balance < 0)
        {
            char error[200];
            snprintf(error, sizeof(error), "Line #%d - Negative balance in account %d\n", i + 1, account);
            strncat(user_errors, error, sizeof(user_errors) - strlen(user_errors) - 1);
        }
        else
        {
            insert_user(account, name, balance);
            total_users_loaded++;
            users_per_thread[info->id - 1]++;
        }
        pthread_mutex_unlock(&lock);
    }

    pthread_exit(NULL);
    return NULL;
}

void *load_operations(void *arg)
{
    ThreadInfo *info = (ThreadInfo *)arg;

    for (int i = info->start_index; i < info->end_index; i++)
    {
        json_t *value = json_array_get(info->root, i);

        int operation_type = json_integer_value(json_object_get(value, "operacion"));
        int account1 = json_integer_value(json_object_get(value, "cuenta1"));
        int account2 = json_integer_value(json_object_get(value, "cuenta2"));
        float amount = json_real_value(json_object_get(value, "monto"));

        pthread_mutex_lock(&lock);
        if (!account_exists(account1) || (operation_type == 3 && !account_exists(account2)))
        {
            char error[200];
            snprintf(error, sizeof(error), "Line #%d - Account %d or %d does not exist\n", i + 1, account1, account2);
            strncat(operation_errors, error, sizeof(operation_errors) - strlen(operation_errors) - 1);
        }
        else
        {
            if (operation_type == 1)
            {
                deposit(account1, amount);
            }
            else if (operation_type == 2)
            {
                withdraw(account1, amount, i + 1);
            }
            else if (operation_type == 3)
            {
                transfer(account1, account2, amount, i + 1);
            }
            total_operations_loaded++;
            operations_per_thread[info->id - 1]++;
        }
        pthread_mutex_unlock(&lock);
    }

    pthread_exit(NULL);
    return NULL;
}

void generate_user_report()
{
    char filename[100];
    char date[100];
    time_t now;
    struct tm *info_time;
    time(&now);
    info_time = localtime(&now);
    strftime(filename, sizeof(filename), "user_load_%Y_%m_%d-%H_%M_%S.log", info_time);
    strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", info_time);

    FILE *report = fopen(filename, "w");
    if (report)
    {
        fprintf(report, "Date: %s\n", date);
        fprintf(report, "Total users loaded: %d\n", total_users_loaded);
        for (int i = 0; i < THREADS_USERS; i++)
        {
            fprintf(report, "Thread %d: %d users\n", i + 1, users_per_thread[i]);
        }
        fprintf(report, "Errors:\n%s", user_errors);
        fclose(report);
    }
}

void generate_operation_report()
{
    char filename[100];
    char date[100];
    time_t now;
    struct tm *info_time;
    time(&now);
    info_time = localtime(&now);
    strftime(filename, sizeof(filename), "operations_%Y_%m_%d-%H_%M_%S.log", info_time);
    strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", info_time);

    FILE *report = fopen(filename, "w");
    if (report)
    {
        fprintf(report, "Date: %s\n", date);
        fprintf(report, "Total operations loaded: %d\n", total_operations_loaded);
        for (int i = 0; i < THREADS_OPERATIONS; i++)
        {
            fprintf(report, "Thread %d: %d operations\n", i + 1, operations_per_thread[i]);
        }
        fprintf(report, "Errors:\n%s", operation_errors);
        fclose(report);
    }
}

void generate_balance_report()
{
    User *current = user_list;
    FILE *report = fopen("balance.json", "w");
    if (report)
    {
        json_t *root = json_array();
        while (current != NULL)
        {
            json_t *user = json_object();
            json_object_set_new(user, "no_cuenta", json_integer(current->account_number));
            json_object_set_new(user, "nombre", json_string(current->name));
            json_object_set_new(user, "saldo", json_real(current->balance));
            json_array_append_new(root, user);
            current = current->next;
        }
        json_dumpf(root, report, JSON_INDENT(2));
        json_decref(root);
        fclose(report);
    }
}

void load_users_multithreaded(const char *filename)
{
    json_error_t error;
    json_t *root = json_load_file(filename, 0, &error);
    if (!root)
    {
        fprintf(stderr, "Error loading JSON file: %s\n", error.text);
        return;
    }

    size_t total_users = json_array_size(root);
    size_t chunk_size = total_users / THREADS_USERS;

    pthread_t threads[THREADS_USERS];
    ThreadInfo info[THREADS_USERS];
    for (int i = 0; i < THREADS_USERS; i++)
    {
        info[i].id = i + 1;
        info[i].root = root;
        info[i].start_index = i * chunk_size;
        info[i].end_index = (i == THREADS_USERS - 1) ? total_users : (i + 1) * chunk_size;
        pthread_create(&threads[i], NULL, load_users, &info[i]);
    }

    for (int i = 0; i < THREADS_USERS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    json_decref(root);
    generate_user_report();
}

void load_operations_multithreaded(const char *filename)
{
    json_error_t error;
    json_t *root = json_load_file(filename, 0, &error);
    if (!root)
    {
        fprintf(stderr, "Error loading JSON file: %s\n", error.text);
        return;
    }

    size_t total_operations = json_array_size(root);
    size_t chunk_size = total_operations / THREADS_OPERATIONS;

    pthread_t threads[THREADS_OPERATIONS];
    ThreadInfo info[THREADS_OPERATIONS];
    for (int i = 0; i < THREADS_OPERATIONS; i++)
    {
        info[i].id = i + 1;
        info[i].root = root;
        info[i].start_index = i * chunk_size;
        info[i].end_index = (i == THREADS_OPERATIONS - 1) ? total_operations : (i + 1) * chunk_size;
        pthread_create(&threads[i], NULL, load_operations, &info[i]);
    }

    for (int i = 0; i < THREADS_OPERATIONS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    json_decref(root);
    generate_operation_report();
}

void show_menu()
{
    printf("1. Load users\n");
    printf("2. Load operations\n");
    printf("3. Individual operations\n");
    printf("4. Generate balance report\n");
    printf("5. Exit\n");
}

void handle_user_input()
{
    int option, account1, account2;
    float amount;

    while (1)
    {
        show_menu();
        scanf("%d", &option);

        switch (option)
        {
        case 1:
            load_users_multithreaded("users.json");
            break;
        case 2:
            load_operations_multithreaded("operations.json");
            break;
        case 3:
            printf("1. Deposit\n2. Withdraw\n3. Transfer\n4. Check account\n");
            scanf("%d", &option);
            switch (option)
            {
            case 1:
                printf("Account: ");
                scanf("%d", &account1);
                printf("Amount: ");
                scanf("%f", &amount);
                if (account_exists(account1))
                {
                    deposit(account1, amount);
                }
                else
                {
                    printf("Account not found\n");
                }
                break;
            case 2:
                printf("Account: ");
                scanf("%d", &account1);
                printf("Amount: ");
                scanf("%f", &amount);
                if (account_exists(account1))
                {
                    withdraw(account1, amount, 0);
                }
                else
                {
                    printf("Account not found\n");
                }
                break;
            case 3:
                printf("Source account: ");
                scanf("%d", &account1);
                printf("Destination account: ");
                scanf("%d", &account2);
                printf("Amount: ");
                scanf("%f", &amount);
                if (account_exists(account1) && account_exists(account2))
                {
                    transfer(account1, account2, amount, 0);
                }
                else
                {
                    printf("Account not found\n");
                }
                break;
            case 4:
                printf("Account: ");
                scanf("%d", &account1);
                User *user = get_user(account1);
                if (user)
                {
                    printf("Account: %d, Name: %s, Balance: %.2f\n", user->account_number, user->name, user->balance);
                }
                else
                {
                    printf("Account not found\n");
                }
                break;
            }
            break;
        case 4:
            generate_balance_report();
            break;
        case 5:
            return;
        default:
            printf("Invalid option\n");
        }
    }
}

int main()
{
    pthread_mutex_init(&lock, NULL);

    handle_user_input();

    pthread_mutex_destroy(&lock);
    return 0;
}
