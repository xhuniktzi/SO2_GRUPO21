# Practica 1 - Manual Técnico

- Sistemas Operativos 2
- 201612174 - Alberto Gabriel Reyes Ning 
- 201900000 - Xhunik Nikol Miguel 

## Contenido

- [Descripción General](#descripción-general)
- [Requisitos del Sistema](#requisitos-del-sistema)
- [Componentes del Programa](#componentes-del-programa)
  - [child_proc.c](#child_procc)
  - [parent_proc.c](#parent_procc)
  - [Scripts de Utilidad](#scripts-de-utilidad)
- [Instalación y Ejecución](#instalación-y-ejecución)
- [Notas Importantes](#notas-importantes)
- [Contribuciones y Licencia](#contribuciones-y-licencia)

## Descripción General

El programa está diseñado para monitorear y registrar las llamadas al sistema realizadas por procesos hijos mediante el uso de herramientas de bajo nivel en Linux, específicamente SystemTap. El programa consta de dos componentes ejecutables que manejan la creación de procesos, la ejecución de llamadas al sistema y la captura de estas llamadas para su análisis y registro.

## Requisitos del Sistema

- **Sistema Operativo**: Linux
- **Compilador**: GCC
- **Herramientas adicionales**:
  - SystemTap instalado y configurado

## Componentes del Programa

### child_proc.c

- Este script demuestra cómo interactuar con archivos y manejar señales en un programa en C, integrando conceptos de generación de contenido aleatorio y operaciones de I/O básicas. Es útil para entender las operaciones a nivel de sistema y cómo pueden ser monitoreadas y controladas en un contexto de sistemas operativos como Linux.
- **Funcionalidad**: Realiza operaciones de archivo (open, write, read) sobre `practica1.txt` y registra estas acciones mediante llamadas al sistema.
- **Manejo de señales**: Captura SIGINT para una terminación limpia.
- **Generación de contenido**: Crea y escribe en archivos basándose en caracteres alfanuméricos aleatorios.

#### Funciones
Inicialización:

- srand(time(NULL) + getpid()): Inicializa el generador de números aleatorios con una semilla única, combinando el tiempo actual y el ID del proceso.
- fdch = open(DATA_FILE, O_RDWR, 0777): Abre (o crea si no existe) el archivo practica1.txt con permisos de lectura y escritura para todos los usuarios.
- signal(SIGINT, ctrlc_handler): Asigna la función ctrlc_handler como el manejador de la señal SIGINT.

```c
srand(time(NULL) + getpid());
fdch = open(DATA_FILE, O_RDWR, 0777);
signal(SIGINT, ctrlc_handler);
```

Main:

- Main se ejecuta mientras sigint_received sea 0.
- Dentro del bucle, se genera un tiempo de espera aleatorio y una operación aleatoria cada ciclo.
- Operaciones:
  - Escritura (op == 1): Genera una cadena aleatoria de tamaño BUFFER_SIZE y la escribe en el archivo.
  - Lectura (op == 2): Lee BUFFER_SIZE caracteres del archivo en un buffer.
  - Posicionamiento (op == 3): Reubica el puntero del archivo al inicio usando lseek.
  - sleep(time): Pausa la ejecución del proceso por un número aleatorio de segundos, simulando intervalos irregulares entre operaciones.



```c
    while (!sigint_received) {
        int time = rand() % 3 + 1;
        int op = rand() % 3 + 1;

        if (op == 1) {
            char random_string[BUFFER_SIZE];
            for (int i = 0; i < BUFFER_SIZE; ++i) {
                random_string[i] = random_char();
            }
            write(fdch, random_string, BUFFER_SIZE);
        } else if (op == 2) {
            char buff[BUFFER_SIZE];
            read(fdch, buff, BUFFER_SIZE);
        } else {
            lseek(fdch, 0, SEEK_SET);
        }
        sleep(time);
    }
```

### parent_proc.c

- Este parte ilustra cómo manejar múltiples procesos y cómo interactuar con herramientas de bajo nivel en Linux para monitorear y registrar el comportamiento del sistema.
- **Funcionalidad**: Inicia procesos hijos, monitoriza sus llamadas al sistema usando SystemTap y registra estas llamadas en `syscalls.log`.
- **Manejo de señales**: Captura SIGINT para terminar la monitorización y proporcionar un resumen de las llamadas al sistema interceptadas.

#### Funciones
handle_sigint(int sig):

- Manejar la señal SIGINT para terminar el programa de forma controlada.
- Establece la variable sigint_received a 1, indicando la recepción de la señal.

```c
void handle_sigint(int signal) {
    sigint_received = 1; 
}
```

update_stats(const char *buffer):

- Actualizar las estadísticas de llamadas al sistema basadas en la información recibida.
- Extrae y cuenta las llamadas read, write, y lseek de un buffer de texto, actualizando la estructura de estadísticas.

```c
void update_stats(const char *buffer) {
    char action[16];
    const char *ptr = buffer;
    
    while (sscanf(ptr, "%15s", action) == 1) {
        if (strcmp(action, "read") == 0) {
            stats.read_count++;
        } else if (strcmp(action, "lseek") == 0) {
            stats.open_count++;
        } else if (strcmp(action, "write") == 0) {
            stats.write_count++;
        }
        stats.syscall_count = stats.read_count + stats.open_count + stats.write_count;
        ptr += strlen(action) + 1;
    }
}
```

finalize_stats(int pid1, int pid2, int monitor, int fd):

- Finalizar el programa imprimiendo estadísticas y cerrando procesos y archivos adecuadamente.
- Lee el log de llamadas al sistema, actualiza y muestra estadísticas finales, y termina todos los procesos relacionados.

```c
void finalize_stats(int pid1, int pid2, int monitor, int fd) {
    printf("\nFinalizando ejecución...\n");
    char buff[1024];
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buff, sizeof(buff))) > 0) {
        update_stats(buff);
    }

    printf("Llamadas totales:\t%d\n", stats.syscall_count);
    printf("Llamadas write:\t\t%d\n", stats.write_count);
    printf("Llamadas read:\t\t%d\n", stats.read_count);
    printf("Llamadas seek:\t\t%d\n", stats.open_count);
    close(fd);
    kill(pid1, SIGKILL);
    kill(pid2, SIGKILL);
    kill(monitor, SIGKILL);
    exit(0);
}
```

log_syscall(const char *syscall_info):

- Registrar información de llamadas al sistema en un archivo.
- Abre el archivo de log, escribe la información, y lo cierra.

```c
void log_syscall(const char *syscall_info) {
    FILE *log = fopen(LOG_FILE, "a");
    if (log == NULL) {
        perror("Failed to open log file");
        exit(1);
    }

    fprintf(log, "%s\n", syscall_info);
    fclose(log);
}
```

monitor_syscalls(int pid1, int pid2):

- Monitorizar las llamadas al sistema de los procesos hijos usando SystemTap.
- Ejecuta un comando de SystemTap que redirige su salida al archivo de log.

```c
void monitor_syscalls(int pid1, int pid2) {
    char command[512];
    snprintf(command, sizeof(command), "stap monitor.stp %d %d > " LOG_FILE,
             pid1, pid2);
    system(command);
}
```

main():

- Crea dos procesos hijos que ejecutan el binario child_proc.bin
- Crea un proceso para monitorizar las llamadas al sistema.
- Espera hasta recibir una señal SIGINT para proceder con la finalización.
- Invoca finalize_stats para procesar las estadísticas y terminar el programa.

```c
int main() {
    signal(SIGINT, handle_sigint);

    FILE *log = fopen(LOG_FILE, "w");
    if (log == NULL) {
        perror("Failed to open log file");
        return 1;
    }
    fclose(log);

    int fdch = open(DATA_FILE, O_CREAT | O_TRUNC, 0777);
    close(fdch);

    if (pipe(pipe_fd) == -1) {
        perror("pipe failed");
        return 1;
    }

    pid_t pid1 = fork();
    switch (pid1) {
        case -1:
            perror("fork failed");
            return 1;
        case 0:
            execl("./child_proc.bin", "./child_proc.bin", NULL);
            perror("execl failed");
            return 1;
        default:
            break;
    }

    pid_t pid2 = fork();
    switch (pid2) {
        case -1:
            perror("fork failed");
            return 1;
        case 0:
            execl("./child_proc.bin", "./child_proc.bin", NULL);
            perror("execl failed");
            return 1;
        default:
            break;
    }

    pid_t monitor = fork();
    switch (monitor) {
        case -1:
            perror("fork failed");
            return 1;
        case 0:
            monitor_syscalls(pid1, pid2);
            break;
        default:
            while (!sigint_received) {
                pause();
            }
            finalize_stats(pid1, pid2, monitor, open(LOG_FILE, O_RDONLY));
            break;
    }
    return 0;
}
```

### Scripts de Utilidad

- **compile.sh**: Compila los códigos fuente en C a binarios y establece los permisos necesarios.
- **clean.sh**: Limpia los binarios y archivos de registro.

## Instalación y Ejecución

1. **Compilar el programa**:
   ```bash
   ./compile.sh
   ```

2. **Ejecutar el proceso principal:**
   ```bash
   ./parent_proc.bin
   ```

3. **Limpiar el entorno después de la ejecución:**
   ```bash
   ./clean.sh
   ```

## Notas:
- SystemTap requiere privilegios de administrador para su ejecución.
- Asegúrate de que los ejecutables de los procesos hijo estén en el mismo directorio que el proceso principal para evitar errores de ejecución.
