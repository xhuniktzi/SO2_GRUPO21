# Manual Técnico: Sistema de Gestión de Operaciones Bancarias con Multithreading

## Descripción General

Este manual técnico describe la implementación de un sistema de gestión de operaciones bancarias que utiliza multithreading para mejorar la eficiencia en la carga y procesamiento de datos. Se explican los conceptos aplicados y se detalla cómo se implementaron en la práctica.

## Archivos del Proyecto

### 1. `clean.sh`

Este script elimina todos los archivos de log en el directorio actual.

```sh
#! /bin/bash

rm *.log
```

### 2. `compile.sh`

Este script compila los archivos fuente de C en un ejecutable llamado `bank_operations.bin` y le otorga permisos de ejecución.

```sh
#! /bin/bash

gcc -o bank_operations.bin main.c user_operations.c file_operations.c globals.c -lpthread -ljansson
chmod +x bank_operations.bin
```

## Implementación de Multithreading

### Creación de Hilos

Para manejar múltiples tareas simultáneamente, se utiliza la biblioteca POSIX Threads (`pthread`). La creación de hilos se realiza con la función `pthread_create`, la cual toma como argumentos el identificador del hilo, los atributos del hilo (normalmente `NULL`), la función que ejecutará el hilo y los argumentos para dicha función.

```c
pthread_t threads[THREADS_USERS];
ThreadInfo info[THREADS_USERS];
for (int i = 0; i < THREADS_USERS; i++) {
    info[i].id = i + 1;
    info[i].root = root;
    info[i].start_index = i * chunk_size;
    info[i].end_index = (i == THREADS_USERS - 1) ? total_users : (i + 1) * chunk_size;
    pthread_create(&threads[i], NULL, load_users, &info[i]);
}
```

### Sincronización de Hilos

Para evitar condiciones de carrera donde múltiples hilos acceden y modifican recursos compartidos simultáneamente, se utilizan mutex. Los mutex aseguran que solo un hilo a la vez pueda ejecutar una sección crítica del código.

- **Inicialización de Mutex**:

  ```c
  pthread_mutex_t lock;
  pthread_mutex_init(&lock, NULL);
  ```

- **Uso de Mutex en Sección Crítica**:

  ```c
  pthread_mutex_lock(&lock);
  // Sección crítica
  pthread_mutex_unlock(&lock);
  ```

### Unión de Hilos

La función `pthread_join` se utiliza para esperar a que los hilos secundarios terminen su ejecución antes de que el hilo principal continúe. Esto garantiza que todas las tareas asignadas a los hilos se completen.

```c
for (int i = 0; i < THREADS_USERS; i++) {
    pthread_join(threads[i], NULL);
}
```

### Estructura de Información del Hilo

Para pasar múltiples parámetros a los hilos, se utiliza una estructura que contiene toda la información necesaria. En este proyecto, se utiliza la estructura `ThreadInfo`.

#### Definición de Estructura

```c
typedef struct ThreadInfo {
    int id;
    json_t *root;
    int start_index;
    int end_index;
} ThreadInfo;
```

### Carga Concurrente de Datos

Para mejorar la eficiencia, los datos de usuarios y operaciones se cargan de manera concurrente utilizando múltiples hilos. Cada hilo procesa una parte del archivo JSON, dividiendo el trabajo en chunks.

- **División de Tareas**:

  ```c
  size_t chunk_size = total_users / THREADS_USERS;
  ```

- **Creación y Asignación de Hilos**:

  ```c
  pthread_t threads[THREADS_USERS];
  ThreadInfo info[THREADS_USERS];
  for (int i = 0; i < THREADS_USERS; i++) {
      info[i].id = i + 1;
      info[i].root = root;
      info[i].start_index = i * chunk_size;
      info[i].end_index = (i == THREADS_USERS - 1) ? total_users : (i + 1) * chunk_size;
      pthread_create(&threads[i], NULL, load_users, &info[i]);
  }
  ```

### Sección Crítica

Las secciones críticas son partes del código donde los hilos acceden y modifican datos compartidos, como las listas de usuarios y los contadores de operaciones. Estas secciones están protegidas por mutex para asegurar la integridad de los datos.

```c
pthread_mutex_lock(&lock);
if (account_exists(account)) {
    // Manejo de cuenta existente
} else {
    // Manejo de nueva cuenta
}
pthread_mutex_unlock(&lock);
```

### Función de Salida del Hilo

Cada hilo termina su ejecución llamando a `pthread_exit`, lo que permite limpiar los recursos asignados al hilo.

```c
void *load_users(void *arg) {
    // Código del hilo
    pthread_exit(NULL);
}
```

## Ejemplo Completo: `load_users_multithreaded`

A continuación, se muestra cómo se integran estos conceptos en la función `load_users_multithreaded`, que carga usuarios desde un archivo JSON utilizando múltiples hilos.

### Implementación

```c
void load_users_multithreaded(const char *filename) {
    json_error_t error;
    json_t *root = json_load_file(filename, 0, &error);
    if (!root) {
        fprintf(stderr, "Error cargando archivo JSON: %s\n", error.text);
        return;
    }

    size_t total_users = json_array_size(root);
    size_t chunk_size = total_users / THREADS_USERS;

    pthread_t threads[THREADS_USERS];
    ThreadInfo info[THREADS_USERS];
    for (int i = 0; i < THREADS_USERS; i++) {
        info[i].id = i + 1;
        info[i].root = root;
        info[i].start_index = i * chunk_size;
        info[i].end_index = (i == THREADS_USERS - 1) ? total_users : (i + 1) * chunk_size;
        pthread_create(&threads[i], NULL, load_users, &info[i]);
    }

    for (int i = 0; i < THREADS_USERS; i++) {
        pthread_join(threads[i], NULL);
    }

    json_decref(root);
    generate_user_report();
}
```

En este ejemplo, se divide la carga de usuarios en partes manejadas por diferentes hilos. Cada hilo procesa una porción del archivo JSON, y los mutex aseguran que las operaciones de modificación de datos compartidos sean seguras. Finalmente, el hilo principal espera a que todos los hilos secundarios terminen antes de generar un reporte de usuarios.

## Conclusión

Este proyecto muestra cómo utilizar multithreading en C para mejorar la eficiencia de la carga y procesamiento de datos en un sistema de gestión de operaciones bancarias. Al dividir las tareas entre múltiples hilos y usar mutex para sincronizar el acceso a recursos compartidos, se logra un sistema más rápido y robusto.
