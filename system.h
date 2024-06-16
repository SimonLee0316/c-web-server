#ifndef SYSTEM_H
#define SYSTEM_H

void create_pidfile(const char *pid_file, int *pids, int num_pids);
void read_pidfile(const char *pid_file, int *pids, int *num_pids);

#endif // SYSTEM_H
