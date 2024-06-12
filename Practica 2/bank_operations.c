#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <jansson.h>
#include <time.h>

#define MAX_USERS 1000
#define MAX_OPERATIONS 1000

typedef struct {
    int account_number;
    char name[100];
    double balance;
} User;

typedef struct {
    int operation;  // 1 -> Deposito, 2 -> Retiro, 3 -> Transferencia
    int account1;
    int account2;
    double amount;
} Operation;

User users[MAX_USERS];
Operation operations[MAX_OPERATIONS];

pthread_mutex_t lock;

int user_count = 0;
int operation_count = 0;

char erroresUsuarios[10000];
char erroresOperaciones[10000];

void check_and_create_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        file = fopen(filename, "w");
        if (!file) {
            perror("Cannot create file");
            exit(EXIT_FAILURE);
        }
    }
    fclose(file);
}

void* load_users(void* filename) {
    check_and_create_file((char*)filename);

    FILE* file = fopen((char*)filename, "r");
    if (!file) {
        perror("Cannot open file");
        return NULL;
    }

    json_error_t error;
    json_t* root = json_loadf(file, 0, &error);
    fclose(file);

    if (!root) {
        fprintf(stderr, "JSON error: %s\n", error.text);
        return NULL;
    }

    size_t index;
    json_t* value;
    json_array_foreach(root, index, value) {
        pthread_mutex_lock(&lock);

        if (user_count >= MAX_USERS) {
            pthread_mutex_unlock(&lock);
            break;
        }

        int no_cuenta = json_integer_value(json_object_get(value, "no_cuenta"));
        const char *nombre = json_string_value(json_object_get(value, "nombre"));
        double saldo = json_real_value(json_object_get(value, "saldo"));

        if (no_cuenta && nombre && saldo >= 0) {
            users[user_count].account_number = no_cuenta;
            strcpy(users[user_count].name, nombre);
            users[user_count].balance = saldo;
            user_count++;
        } else {
            sprintf(erroresUsuarios + strlen(erroresUsuarios), "Error en la línea %zu\n", index + 1);
        }

        pthread_mutex_unlock(&lock);
    }

    json_decref(root);
    return NULL;
}

void load_users_multithreaded(const char* filename) {
    pthread_t threads[3];
    for (int i = 0; i < 3; ++i) {
        pthread_create(&threads[i], NULL, load_users, (void*)filename);
    }

    for (int i = 0; i < 3; ++i) {
        pthread_join(threads[i], NULL);
    }
}

void deposit(int account_number, double amount) {
    pthread_mutex_lock(&lock);
    for (int i = 0; i < user_count; ++i) {
        if (users[i].account_number == account_number) {
            users[i].balance += amount;
            pthread_mutex_unlock(&lock);
            return;
        }
    }
    pthread_mutex_unlock(&lock);
}

void withdraw(int account_number, double amount, int line) {
    pthread_mutex_lock(&lock);
    for (int i = 0; i < user_count; ++i) {
        if (users[i].account_number == account_number) {
            if (users[i].balance >= amount) {
                users[i].balance -= amount;
            } else {
                sprintf(erroresOperaciones + strlen(erroresOperaciones), "Línea #%d - Saldo insuficiente en cuenta %d\n", line, account_number);
            }
            pthread_mutex_unlock(&lock);
            return;
        }
    }
    pthread_mutex_unlock(&lock);
}

void transfer(int account1, int account2, double amount, int line) {
    pthread_mutex_lock(&lock);
    User *user1 = NULL, *user2 = NULL;
    for (int i = 0; i < user_count; ++i) {
        if (users[i].account_number == account1) {
            user1 = &users[i];
        }
        if (users[i].account_number == account2) {
            user2 = &users[i];
        }
    }

    if (user1 && user2) {
        if (user1->balance >= amount) {
            user1->balance -= amount;
            user2->balance += amount;
        } else {
            sprintf(erroresOperaciones + strlen(erroresOperaciones), "Línea #%d - Saldo insuficiente en cuenta %d\n", line, account1);
        }
    } else {
        sprintf(erroresOperaciones + strlen(erroresOperaciones), "Línea #%d - Cuenta no encontrada\n", line);
    }
    pthread_mutex_unlock(&lock);
}

void* load_operations(void* filename) {
    check_and_create_file((char*)filename);

    FILE* file = fopen((char*)filename, "r");
    if (!file) {
        perror("Cannot open file");
        return NULL;
    }

    json_error_t error;
    json_t* root = json_loadf(file, 0, &error);
    fclose(file);

    if (!root) {
        fprintf(stderr, "JSON error: %s\n", error.text);
        return NULL;
    }

    size_t index;
    json_t* value;
    json_array_foreach(root, index, value) {
        pthread_mutex_lock(&lock);

        if (operation_count >= MAX_OPERATIONS) {
            pthread_mutex_unlock(&lock);
            break;
        }

        operations[operation_count].operation = json_integer_value(json_object_get(value, "operacion"));
        operations[operation_count].account1 = json_integer_value(json_object_get(value, "cuenta1"));
        operations[operation_count].account2 = json_integer_value(json_object_get(value, "cuenta2"));
        operations[operation_count].amount = json_real_value(json_object_get(value, "monto"));

        switch (operations[operation_count].operation) {
            case 1:
                deposit(operations[operation_count].account1, operations[operation_count].amount);
                break;
            case 2:
                withdraw(operations[operation_count].account1, operations[operation_count].amount, index + 1);
                break;
            case 3:
                transfer(operations[operation_count].account1, operations[operation_count].account2, operations[operation_count].amount, index + 1);
                break;
            default:
                sprintf(erroresOperaciones + strlen(erroresOperaciones), "Línea #%zu - Operación no válida\n", index + 1);
        }

        operation_count++;
        pthread_mutex_unlock(&lock);
    }

    json_decref(root);
    return NULL;
}

void load_operations_multithreaded(const char* filename) {
    pthread_t threads[4];
    for (int i = 0; i < 4; ++i) {
        pthread_create(&threads[i], NULL, load_operations, (void*)filename);
    }

    for (int i = 0; i < 4; ++i) {
        pthread_join(threads[i], NULL);
    }
}

void estadoDeCuenta() {
    FILE *reporte = fopen("Estado_de_cuenta.json", "w");
    if (reporte == NULL) {
        perror("Error al abrir el archivo");
        return;
    }

    json_t *jarray = json_array();
    for (int i = 0; i < user_count; ++i) {
        json_t *jobj = json_object();
        json_object_set_new(jobj, "no_cuenta", json_integer(users[i].account_number));
        json_object_set_new(jobj, "nombre", json_string(users[i].name));
        json_object_set_new(jobj, "saldo", json_real(users[i].balance));
        json_array_append_new(jarray, jobj);
    }

    fprintf(reporte, "%s\n", json_dumps(jarray, JSON_INDENT(4)));
    fclose(reporte);
    json_decref(jarray);
}

void show_menu() {
    printf("1. Deposito\n");
    printf("2. Retiro\n");
    printf("3. Transferencia\n");
    printf("4. Consultar cuenta\n");
    printf("5. Generar estado de cuenta\n");
    printf("6. Salir\n");
}

void handle_user_input() {
    int choice, account1, account2;
    double amount;

    while (1) {
        show_menu();
        printf("Ingrese una opción: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                printf("Ingrese el numero de cuenta: ");
                scanf("%d", &account1);
                printf("Ingrese el monto a depositar: ");
                scanf("%lf", &amount);
                deposit(account1, amount);
                break;
            case 2:
                printf("Ingrese el numero de cuenta: ");
                scanf("%d", &account1);
                printf("Ingrese el monto a retirar: ");
                scanf("%lf", &amount);
                withdraw(account1, amount, 0);
                break;
            case 3:
                printf("Ingrese el numero de cuenta origen: ");
                scanf("%d", &account1);
                printf("Ingrese el numero de cuenta destino: ");
                scanf("%d", &account2);
                printf("Ingrese el monto a transferir: ");
                scanf("%lf", &amount);
                transfer(account1, account2, amount, 0);
                break;
            case 4:
                printf("Ingrese el numero de cuenta: ");
                scanf("%d", &account1);
                for (int i = 0; i < user_count; ++i) {
                    if (users[i].account_number == account1) {
                        printf("Cuenta: %d, Nombre: %s, Saldo: %.2f\n", users[i].account_number, users[i].name, users[i].balance);
                    }
                }
                break;
            case 5:
                estadoDeCuenta();
                break;
            case 6:
                return;
            default:
                printf("Opción no válida\n");
        }
    }
}

int main() {
    pthread_mutex_init(&lock, NULL);

    const char* user_file = "users.json";
    const char* operations_file = "operations.json";

    load_users_multithreaded(user_file);
    load_operations_multithreaded(operations_file);

    handle_user_input();

    pthread_mutex_destroy(&lock);
    return 0;
}
