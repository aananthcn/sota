#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include "unixcommon.h"

int daemon_proc;

pid_t Fork(void)
{
	pid_t pid;

	if((pid = fork()) == -1)
		err_sys("fork error");

	return (pid);
}


void Close(int fd)
{
        if(close(fd) == -1)
                err_sys("close error");
}


char* Fgets(char *ptr, int n, FILE *stream)
{
	char    *rptr;

	if ( (rptr = fgets(ptr, n, stream)) == NULL && ferror(stream))
		err_sys("fgets error");

	return (rptr);
}


void Fputs(const char *ptr, FILE *stream)
{
	if (fputs(ptr, stream) == EOF)
		err_sys("fputs error");
}


void sig_chld(int signo)
{
        pid_t   pid;
        int     stat;

        pid = wait(&stat);
        printf("child %d terminated\n", pid);
        return;
}


/* Print message and return to caller
 * Caller specifies "errnoflag" and "level" */

void err_doit(int errnoflag, int level, const char *fmt, va_list ap)
{
	int     errno_save, n;
	char    buf[MAXLINE + 1];

	errno_save = errno;             /* value caller might want printed */
#ifdef  HAVE_VSNPRINTF
	vsnprintf(buf, MAXLINE, fmt, ap);       /* safe */
#else
	vsprintf(buf, fmt, ap);                                 /* not safe */
#endif
	n = strlen(buf);
	if (errnoflag)
		snprintf(buf + n, MAXLINE - n, ": %s", strerror(errno_save));
	strcat(buf, "\n");

	if (daemon_proc) {
		syslog(level, buf);
	} else {
		fflush(stdout);         /* in case stdout and stderr are the same */
		fputs(buf, stderr);
		fflush(stderr);
	}

	return;
}


void err_sys(const char *fmt, ...)
{
        va_list         ap;

        va_start(ap, fmt);
        err_doit(1, LOG_ERR, fmt, ap);
        va_end(ap);

        exit(1);
}

void err_quit(const char *fmt, ...)
{
	va_list         ap;

	va_start(ap, fmt);
	err_doit(0, LOG_ERR, fmt, ap);
	va_end(ap);
	exit(1);
}

