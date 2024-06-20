#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>
#include <time.h>

void log_to_db(const char *pid, const char *name, const char *call, long size, const char *timestamp) {
    MYSQL *conn;
    conn = mysql_init(NULL);
    if (conn == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        return;
    }

    if (mysql_real_connect(conn, "192.168.191.1", "vmuser", "1234", "memory_monitor", 0, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect() failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return;
    }

    if (mysql_query(conn, "USE memory_monitor")) {
        fprintf(stderr, "USE memory_monitor failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return;
    }

    char query[512];
    snprintf(query, sizeof(query), 
             "INSERT INTO memory_log (pid, process_name, call_type, memory_size, timestamp) VALUES ('%s', '%s', '%s', %ld, '%s')",
             pid, name, call, size, timestamp);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "INSERT failed: %s\n", mysql_error(conn));
    } else {
        printf("Data inserted successfully\n");
    }

    mysql_close(conn);
}

void get_current_timestamp(char *buffer, size_t buffer_size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", t);
}

int main() {
    FILE *fp;
    char line[256];

    fp = popen("sudo stap /usr/local/bin/memory_script.stp", "r");
    if (fp == NULL) {
        perror("Failed to run SystemTap script");
        return 1;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        char pid[10], name[255], call[10], timestamp[20];
        long size;

        printf("Raw line: %s\n", line);

        int parsed_items = sscanf(line, "PID:%9[^,],Proceso:%254[^,],%9[^:]:%ld bytes", pid, name, call, &size);
        printf("Parsed items: %d\n", parsed_items);
        printf("pid: %s, name: %s, call: %s, size: %ld\n", pid, name, call, size);

        if (parsed_items == 4) {
            get_current_timestamp(timestamp, sizeof(timestamp));
            printf("Parsed data - timestamp: %s, pid: %s, name: %s, call: %s, size: %ld\n", timestamp, pid, name, call, size);

            char *endptr;
            long pid_value = strtol(pid, &endptr, 10);
            if (*endptr == '\0') {
                log_to_db(pid, name, call, size, timestamp);
            } else {
                fprintf(stderr, "Invalid PID: %s\n", pid);
            }
        } else {
            fprintf(stderr, "Failed to parse line: %s\n", line);
        }
    }

    pclose(fp);
    return 0;
}

