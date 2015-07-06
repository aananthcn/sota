#include <stdarg.h>              /* ANSI C header file */
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <syslog.h>
#include <stdarg.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "tcpcommon.h"
#include "unixcommon.h"

int Socket(int family, int type, int protocol)
{
	int n;

	if((n = socket(family, type, protocol)) < 0)
		err_sys("socket error");

	return (n);
}


void Bind(int fd, const struct sockaddr *sa, socklen_t salen)
{
	if(bind(fd, sa, salen) < 0)
		err_sys("bind error");
}


void Listen(int fd, int backlog)
{
	char *ptr;

	/*4can override 2nd argument with environment variable */
	if((ptr = getenv("LISTENQ")) != NULL)
		backlog = atoi(ptr);

	if(listen(fd, backlog) < 0)
		err_sys("listen error");
}


Sigfunc* Signal(int signo, Sigfunc *func)        /* for our signal() function */
{
        Sigfunc *sigfunc;

        if((sigfunc = signal(signo, func)) == SIG_ERR)
                err_sys("signal error");

	return (sigfunc);
}


/* Read "n" bytes from a descriptor. */
ssize_t readn(int fd, void *vptr, size_t n)
{
	size_t  nleft;
	ssize_t nread;
	char    *ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		printf("Need to receive %d more bytes\n", (int)nleft);
		if ( (nread = read(fd, ptr, nleft)) < 0) {
			if (errno == EINTR)
				nread = 0;              /* and call read() again */
			else
				return(-1);
		} else if (nread == 0)
			break;                          /* EOF */

		nleft -= nread;
		ptr   += nread;
	}
	return (n - nleft);              /* return >= 0 */
}
/* end readn */

ssize_t Readn(int fd, void *ptr, size_t nbytes)
{
	ssize_t n;

	//if((n = readn(fd, ptr, nbytes)) < 0)
	if((n = read(fd, ptr, nbytes)) < 0)
		err_sys("readn error");

	return(n);
}

/* Write "n" bytes to a descriptor. */
ssize_t writen(int fd, const void *vptr, size_t n)
{
	size_t          nleft;
	ssize_t         nwritten;
	const char      *ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
			if (nwritten < 0 && errno == EINTR)
				nwritten = 0;           /* and call write() again */
			else
				return(-1);                     /* error */
		}

		nleft -= nwritten;
		ptr   += nwritten;
	}
	return(n);
}
/* end writen */

void Writen(int fd, void *ptr, size_t nbytes)
{
	if(writen(fd, ptr, nbytes) != nbytes)
		err_sys("writen error");
}


void Connect(int fd, const struct sockaddr *sa, socklen_t salen)
{
	if (connect(fd, sa, salen) < 0)
		err_sys("connect error");
}


void str_echo(int sockfd)
{
        ssize_t n;
	char recvline[MAXLINE];
	printf("Connected to a remote client. ");

        for ( ; ; ) {
		printf("Waiting for client to send msg...\n");
                if((n = Readn(sockfd, recvline, MAXLINE)) == 0)
                        return;         /* connection closed by other end */
		printf("received a frame\n");

                Writen(sockfd, recvline, MAXLINE);
        }
}


void str_cli(FILE *fp, int sockfd)
{
	char sendline[MAXLINE], recvline[MAXLINE];

	while (Fgets(sendline, MAXLINE, fp) != NULL) {

		Writen(sockfd, sendline, strlen(sendline));
		printf("Sent message: \"%s\"\n",sendline);

		if (Readline(sockfd, recvline, MAXLINE) == 0)
			err_quit("str_cli: server terminated prematurely");

		printf("received a frame!!\n");
		Fputs(recvline, stdout);
	}
}



const char* Inet_ntop(int family, const void *addrptr, char *strptr, size_t len)
{
	const char      *ptr;

	if (strptr == NULL)             /* check for old code */
		err_quit("NULL 3rd argument to inet_ntop");
	if ( (ptr = inet_ntop(family, addrptr, strptr, len)) == NULL)
		err_sys("inet_ntop error");             /* sets errno */
	return(ptr);
}

void Inet_pton(int family, const char *strptr, void *addrptr)
{
	int             n;

	if ( (n = inet_pton(family, strptr, addrptr)) < 0)
		err_sys("inet_pton error for %s", strptr);      /* errno set */
	else if (n == 0)
		err_quit("inet_pton error for %s", strptr);     /* errno not set */

	/* nothing to return */
}

