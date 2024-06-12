#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <regex.h>
#include <time.h>
#include <jansson.h>

#define THREADS_USERS 3
#define THREADS_OPERATIONS 4
#define MAX_USERS 1000
#define MAX_OPERATIONS 1000

pthread_mutex_t lock;
int usuariosLeidos;
int usuariosHilo1;
int usuariosHilo2;
int usuariosHilo3;

int operacionesLeidas;
int operacionesHilo1;
int operacionesHilo2;
int operacionesHilo3;
int operacionesHilo4;

struct usuario
{
    int no_cuenta;
    char nombre[100];
    float saldo;
    struct usuario *siguiente;
};
struct usuario *cabezaUsuarios = NULL;

struct operacion
{
    int operacion;
    int cuenta1;
    int cuenta2;
    float monto;
};

char erroresUsuarios[10000];
char erroresOperaciones[10000];

struct usuario usuarios[MAX_USERS];
struct operacion operaciones[MAX_OPERATIONS];

typedef struct hilo_info
{
    int id;
    json_t *root;
    int start_index;
    int end_index;
} HiloInfo;

void insertarUsuario(int numeroCuenta, char nombre[100], float saldo)
{
    struct usuario *nuevoNodo = (struct usuario *)malloc(sizeof(struct usuario));
    nuevoNodo->no_cuenta = numeroCuenta;
    strcpy(nuevoNodo->nombre, nombre);
    nuevoNodo->saldo = saldo;
    nuevoNodo->siguiente = cabezaUsuarios;
    cabezaUsuarios = nuevoNodo;
}

bool existeCuenta(int numeroCuenta)
{
    struct usuario *actual = cabezaUsuarios;
    while (actual != NULL)
    {
        if (actual->no_cuenta == numeroCuenta)
        {
            return true;
        }
        actual = actual->siguiente;
    }
    return false;
}

struct usuario *getUsuario(int numeroCuenta)
{
    struct usuario *actual = cabezaUsuarios;
    while (actual != NULL)
    {
        if (actual->no_cuenta == numeroCuenta)
        {
            return actual;
        }
        actual = actual->siguiente;
    }
    return NULL;
}

void deposito(int cuenta, float monto)
{
    struct usuario *usuario = getUsuario(cuenta);
    usuario->saldo = usuario->saldo + monto;
}

void retiro(int cuenta, float monto, int linea)
{
    struct usuario *usuario = getUsuario(cuenta);
    char error[200];
    if (usuario->saldo - monto >= 0)
    {
        usuario->saldo = usuario->saldo - monto;
    }
    else
    {
        if (linea > 0)
        {
            sprintf(error, "Línea #%d - Saldo insuficiente en cuenta %d\n", linea, cuenta);
            strcat(erroresOperaciones, error);
        }
    }
}

void transferencia(int cuenta1, int cuenta2, float monto, int linea)
{
    struct usuario *origen = getUsuario(cuenta1);
    struct usuario *destino = getUsuario(cuenta2);
    char error[200];
    if (destino != NULL)
    {
        if (origen->saldo - monto >= 0)
        {
            origen->saldo -= monto;
            destino->saldo += monto;
            printf("Transferencia de %d a %d por %.2f completada.\n", cuenta1, cuenta2, monto);
        }
        else
        {
            sprintf(error, "Línea #%d - Saldo insuficiente en cuenta %d\n", linea, cuenta1);
            strcat(erroresOperaciones, error);
        }
    }
    else
    {
        sprintf(error, "Línea #%d - Cuenta destino %d no existe\n", linea, cuenta2);
        strcat(erroresOperaciones, error);
    }
}

void *cargarUsuarios(void *arg)
{
    HiloInfo *info_hilo = (HiloInfo *)arg;

    for (int i = info_hilo->start_index; i < info_hilo->end_index; i++)
    {
        json_t *value = json_array_get(info_hilo->root, i);

        int cuenta = json_integer_value(json_object_get(value, "no_cuenta"));
        const char *nombre = json_string_value(json_object_get(value, "nombre"));
        double saldo = json_real_value(json_object_get(value, "saldo"));

        pthread_mutex_lock(&lock);
        if (existeCuenta(cuenta))
        {
            char error[200];
            sprintf(error, "Línea #%d - Cuenta duplicada %d\n", i + 1, cuenta);
            strcat(erroresUsuarios, error);
        }
        else
        {
            insertarUsuario(cuenta, (char *)nombre, (float)saldo);
            usuariosLeidos++;
            if (info_hilo->id == 1)
            {
                usuariosHilo1++;
            }
            else if (info_hilo->id == 2)
            {
                usuariosHilo2++;
            }
            else
            {
                usuariosHilo3++;
            }
        }
        pthread_mutex_unlock(&lock);
    }

    pthread_exit(NULL);
    return NULL;
}

void *cargarOperaciones(void *arg)
{
    HiloInfo *info_hilo = (HiloInfo *)arg;

    for (int i = info_hilo->start_index; i < info_hilo->end_index; i++)
    {
        json_t *value = json_array_get(info_hilo->root, i);

        int operacion = json_integer_value(json_object_get(value, "operacion"));
        int cuenta1 = json_integer_value(json_object_get(value, "cuenta1"));
        int cuenta2 = json_integer_value(json_object_get(value, "cuenta2"));
        double monto = json_real_value(json_object_get(value, "monto"));

        pthread_mutex_lock(&lock);
        if (!existeCuenta(cuenta1) || (operacion == 3 && !existeCuenta(cuenta2)))
        {
            char error[200];
            sprintf(error, "Línea #%d - Cuenta no existe %d o %d\n", i + 1, cuenta1, cuenta2);
            strcat(erroresOperaciones, error);
        }
        else
        {
            if (operacion == 1)
            {
                deposito(cuenta1, (float)monto);
            }
            else if (operacion == 2)
            {
                retiro(cuenta1, (float)monto, i + 1);
            }
            else if (operacion == 3)
            {
                transferencia(cuenta1, cuenta2, (float)monto, i + 1);
            }
            operacionesLeidas++;
            if (info_hilo->id == 1)
            {
                operacionesHilo1++;
            }
            else if (info_hilo->id == 2)
            {
                operacionesHilo2++;
            }
            else if (info_hilo->id == 3)
            {
                operacionesHilo3++;
            }
            else
            {
                operacionesHilo4++;
            }
        }
        pthread_mutex_unlock(&lock);
    }

    pthread_exit(NULL);
    return NULL;
}

void generarReporteUsuarios()
{
    char nombre[100];
    char fecha[100];
    time_t tiempo_actual;
    struct tm *info_tiempo;
    time(&tiempo_actual);
    info_tiempo = localtime(&tiempo_actual);
    strftime(nombre, sizeof(nombre), "carga_%Y_%m_%d-%H_%M_%S.log", info_tiempo);
    strftime(fecha, sizeof(fecha), "%Y-%m-%d %H:%M:%S", info_tiempo);
    FILE *reporte = fopen(nombre, "w");
    if (reporte)
    {
        fprintf(reporte, "Fecha: %s\n", fecha);
        fprintf(reporte, "Usuarios cargados: %d\n", usuariosLeidos);
        fprintf(reporte, "Hilo 1: %d\n", usuariosHilo1);
        fprintf(reporte, "Hilo 2: %d\n", usuariosHilo2);
        fprintf(reporte, "Hilo 3: %d\n", usuariosHilo3);
        fprintf(reporte, "Errores:\n%s", erroresUsuarios);
        fclose(reporte);
    }
}

void generarReporteOperaciones()
{
    char nombre[100];
    char fecha[100];
    time_t tiempo_actual;
    struct tm *info_tiempo;
    time(&tiempo_actual);
    info_tiempo = localtime(&tiempo_actual);
    strftime(nombre, sizeof(nombre), "operaciones_%Y_%m_%d-%H_%M_%S.log", info_tiempo);
    strftime(fecha, sizeof(fecha), "%Y-%m-%d %H:%M:%S", info_tiempo);
    FILE *reporte = fopen(nombre, "w");
    if (reporte)
    {
        fprintf(reporte, "Fecha: %s\n", fecha);
        fprintf(reporte, "Operaciones cargadas: %d\n", operacionesLeidas);
        fprintf(reporte, "Hilo 1: %d\n", operacionesHilo1);
        fprintf(reporte, "Hilo 2: %d\n", operacionesHilo2);
        fprintf(reporte, "Hilo 3: %d\n", operacionesHilo3);
        fprintf(reporte, "Hilo 4: %d\n", operacionesHilo4);
        fprintf(reporte, "Errores:\n%s", erroresOperaciones);
        fclose(reporte);
    }
}

void estadoDeCuenta()
{
    struct usuario *actual = cabezaUsuarios;
    FILE *reporte = fopen("balance.json", "w");
    if (reporte)
    {
        json_t *root = json_array();
        while (actual != NULL)
        {
            json_t *user = json_object();
            json_object_set_new(user, "no_cuenta", json_integer(actual->no_cuenta));
            json_object_set_new(user, "nombre", json_string(actual->nombre));
            json_object_set_new(user, "saldo", json_real(actual->saldo));
            json_array_append_new(root, user);
            actual = actual->siguiente;
        }
        json_dumpf(root, reporte, JSON_INDENT(2));
        json_decref(root);
        fclose(reporte);
    }
}

void cargarUsuariosMultithreaded(const char *filename)
{
    json_error_t error;
    json_t *root = json_load_file(filename, 0, &error);
    if (!root)
    {
        fprintf(stderr, "Error al cargar archivo JSON: %s\n", error.text);
        return;
    }

    size_t total_usuarios = json_array_size(root);
    size_t chunk_size = total_usuarios / THREADS_USERS;

    pthread_t threads[THREADS_USERS];
    HiloInfo info[THREADS_USERS];
    for (int i = 0; i < THREADS_USERS; i++)
    {
        info[i].id = i + 1;
        info[i].root = root;
        info[i].start_index = i * chunk_size;
        info[i].end_index = (i == THREADS_USERS - 1) ? total_usuarios : (i + 1) * chunk_size;
        pthread_create(&threads[i], NULL, cargarUsuarios, &info[i]);
    }

    for (int i = 0; i < THREADS_USERS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    json_decref(root);
    generarReporteUsuarios();
}

void cargarOperacionesMultithreaded(const char *filename)
{
    json_error_t error;
    json_t *root = json_load_file(filename, 0, &error);
    if (!root)
    {
        fprintf(stderr, "Error al cargar archivo JSON: %s\n", error.text);
        return;
    }

    size_t total_operaciones = json_array_size(root);
    size_t chunk_size = total_operaciones / THREADS_OPERATIONS;

    pthread_t threads[THREADS_OPERATIONS];
    HiloInfo info[THREADS_OPERATIONS];
    for (int i = 0; i < THREADS_OPERATIONS; i++)
    {
        info[i].id = i + 1;
        info[i].root = root;
        info[i].start_index = i * chunk_size;
        info[i].end_index = (i == THREADS_OPERATIONS - 1) ? total_operaciones : (i + 1) * chunk_size;
        pthread_create(&threads[i], NULL, cargarOperaciones, &info[i]);
    }

    for (int i = 0; i < THREADS_OPERATIONS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    json_decref(root);
    generarReporteOperaciones();
}

void mostrarMenu()
{
    printf("1. Cargar usuarios\n");
    printf("2. Cargar operaciones\n");
    printf("3. Operaciones individuales\n");
    printf("4. Generar estado de cuentas\n");
    printf("5. Salir\n");
}

void manejarEntradaUsuario()
{
    int opcion, cuenta1, cuenta2;
    double monto;

    while (1)
    {
        mostrarMenu();
        scanf("%d", &opcion);

        switch (opcion)
        {
        case 1:
            cargarUsuariosMultithreaded("users.json");
            break;
        case 2:
            cargarOperacionesMultithreaded("operations.json");
            break;
        case 3:
            printf("1. Deposito\n2. Retiro\n3. Transferencia\n4. Consultar cuenta\n");
            scanf("%d", &opcion);
            switch (opcion)
            {
            case 1:
                printf("Cuenta: ");
                scanf("%d", &cuenta1);
                printf("Monto: ");
                scanf("%lf", &monto);
                if (existeCuenta(cuenta1))
                {
                    deposito(cuenta1, monto);
                }
                else
                {
                    printf("Cuenta no encontrada\n");
                }
                break;
            case 2:
                printf("Cuenta: ");
                scanf("%d", &cuenta1);
                printf("Monto: ");
                scanf("%lf", &monto);
                if (existeCuenta(cuenta1))
                {
                    retiro(cuenta1, monto, 0);
                }
                else
                {
                    printf("Cuenta no encontrada\n");
                }
                break;
            case 3:
                printf("Cuenta origen: ");
                scanf("%d", &cuenta1);
                printf("Cuenta destino: ");
                scanf("%d", &cuenta2);
                printf("Monto: ");
                scanf("%lf", &monto);
                if (existeCuenta(cuenta1) && existeCuenta(cuenta2))
                {
                    transferencia(cuenta1, cuenta2, monto, 0);
                }
                else
                {
                    printf("Cuenta no encontrada\n");
                }
                break;
            case 4:
                printf("Cuenta: ");
                scanf("%d", &cuenta1);
                struct usuario *usuario = getUsuario(cuenta1);
                if (usuario)
                {
                    printf("Cuenta: %d, Nombre: %s, Saldo: %.2f\n", usuario->no_cuenta, usuario->nombre, usuario->saldo);
                }
                else
                {
                    printf("Cuenta no encontrada\n");
                }
                break;
            }
            break;
        case 4:
            estadoDeCuenta();
            break;
        case 5:
            return;
        default:
            printf("Opción no válida\n");
        }
    }
}

int main()
{
    pthread_mutex_init(&lock, NULL);

    manejarEntradaUsuario();

    pthread_mutex_destroy(&lock);
    return 0;
}
