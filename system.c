#include "system.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void create_pidfile(const char *pid_file, int *pids, int num_pids) {
  FILE *file = fopen(pid_file, "w");
  if (file == NULL) {
    perror("Could not open PID file for writing");
    exit(1);
  }
  for (int i = 0; i < num_pids; i++) {
    fprintf(file, "%d\n", pids[i]);
  }
  fclose(file);
}

void read_pidfile(const char *pid_file, int *pids, int *num_pids) {
  FILE *file = fopen(pid_file, "r");
  if (file == NULL) {
    perror("Could not open PID file for reading");
    return;
  }
  int pid;
  *num_pids = 0;
  while (fscanf(file, "%d", &pid) != EOF) {
    pids[*num_pids] = pid;
    (*num_pids)++;
  }
  fclose(file);
}
