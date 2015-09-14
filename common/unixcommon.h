#ifndef UNIXCOMMON_H
#define UNIXCOMMON_H

#include <errno.h>
#include <unistd.h>
#include <stdio.h>

/* Miscellaneous constants */
#define MAXLINE         4096    /* max text line length */

char* get_filename(char *path);
pid_t Fork(void);
void Close(int fd);
char* Fgets(char *ptr, int n, FILE *stream);

void err_sys(const char *fmt, ...);

#endif
