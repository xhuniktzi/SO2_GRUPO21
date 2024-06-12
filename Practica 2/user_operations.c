#include "headers.h"

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
            snprintf(error, sizeof(error), "Línea #%d - Saldo insuficiente en cuenta %d\n", line_number, account_number);
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
        snprintf(error, sizeof(error), "Línea #%d - Cuenta origen %d no existe\n", line_number, account1);
        strncat(operation_errors, error, sizeof(operation_errors) - strlen(operation_errors) - 1);
        return;
    }
    if (!user2)
    {
        char error[200];
        snprintf(error, sizeof(error), "Línea #%d - Cuenta destino %d no existe\n", line_number, account2);
        strncat(operation_errors, error, sizeof(operation_errors) - strlen(operation_errors) - 1);
        return;
    }
    if (user1->balance < amount)
    {
        char error[200];
        snprintf(error, sizeof(error), "Línea #%d - Saldo insuficiente en cuenta %d\n", line_number, account1);
        strncat(operation_errors, error, sizeof(operation_errors) - strlen(operation_errors) - 1);
        return;
    }
    user1->balance -= amount;
    user2->balance += amount;
}
