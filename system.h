#ifndef SYSTEM_H
#define SYSTEM_H

void create_pidfile(const char *pid_file);
pid_t read_pidfile(const char *pid_file);

#endif // SYSTEM_H
