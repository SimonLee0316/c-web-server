#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "system.h"

void create_pidfile(const char *pid_file) {
    FILE *file = fopen(pid_file, "w");
    if (file == NULL) {
        perror("Could not open PID file for writing");
        exit(1);
    }
    fprintf(file, "%d\n", getpid());
    fclose(file);
}

pid_t read_pidfile(const char *pid_file) {
    FILE *file = fopen(pid_file, "r");
    if (file == NULL) {
        perror("Could not open PID file for reading");
        return -1;
    }
    pid_t pid;
    fscanf(file, "%d", &pid);
    fclose(file);
    return pid;
}

void delete_pidfile()
{
    
}