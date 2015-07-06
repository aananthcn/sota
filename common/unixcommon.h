#ifndef UNIXCOMMON_H
#define UNIXCOMMON_H

#include <errno.h>
#include <unistd.h>

/* Miscellaneous constants */
#define MAXLINE         4096    /* max text line length */

pid_t Fork(void);
void Close(int fd);

void err_sys(const char *fmt, ...);

#endif
