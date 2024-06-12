#include "headers.h"

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
            snprintf(error, sizeof(error), "Línea #%d - Cuenta duplicada %d\n", i + 1, account);
            strncat(user_errors, error, sizeof(user_errors) - strlen(user_errors) - 1);
        }
        else if (balance < 0)
        {
            char error[200];
            snprintf(error, sizeof(error), "Línea #%d - Saldo negativo en cuenta %d\n", i + 1, account);
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
            snprintf(error, sizeof(error), "Línea #%d - La cuenta %d o %d no existe\n", i + 1, account1, account2);
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
    strftime(filename, sizeof(filename), "reporte_carga_usuarios_%Y_%m_%d-%H_%M_%S.log", info_time);
    strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", info_time);

    FILE *report = fopen(filename, "w");
    if (report)
    {
        fprintf(report, "Fecha: %s\n", date);
        fprintf(report, "Total de usuarios cargados: %d\n", total_users_loaded);
        for (int i = 0; i < THREADS_USERS; i++)
        {
            fprintf(report, "Hilo %d: %d usuarios\n", i + 1, users_per_thread[i]);
        }
        fprintf(report, "Errores:\n%s", user_errors);
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
    strftime(filename, sizeof(filename), "reporte_operaciones_%Y_%m_%d-%H_%M_%S.log", info_time);
    strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", info_time);

    FILE *report = fopen(filename, "w");
    if (report)
    {
        fprintf(report, "Fecha: %s\n", date);
        fprintf(report, "Total de operaciones cargadas: %d\n", total_operations_loaded);
        for (int i = 0; i < THREADS_OPERATIONS; i++)
        {
            fprintf(report, "Hilo %d: %d operaciones\n", i + 1, operations_per_thread[i]);
        }
        fprintf(report, "Errores:\n%s", operation_errors);
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
        fprintf(stderr, "Error cargando archivo JSON: %s\n", error.text);
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
        fprintf(stderr, "Error cargando archivo JSON: %s\n", error.text);
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
