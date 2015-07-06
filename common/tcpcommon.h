#ifndef TCPCOMMON_H
#define TCPCOMMON_H


/* Following could be derived from SOMAXCONN in <sys/socket.h>, but many
   kernels still #define it as 5, while actually supporting many more */
#define LISTENQ         1024    /* 2nd argument to listen() */

/* Define some port number that can be used for our examples */
#define SERV_PORT        9877                   /* TCP and UDP */
#define SERV_PORT_STR   "9877"                  /* TCP and UDP */
#define UNIXSTR_PATH    "/tmp/unix.str" /* Unix domain stream */
#define UNIXDG_PATH     "/tmp/unix.dg"  /* Unix domain datagram */

/* Following shortens all the typecasts of pointer arguments: */
#define SA      struct sockaddr

typedef void Sigfunc(int);   /* for signal handlers */


int Socket(int family, int type, int protocol);
void Bind(int fd, const struct sockaddr *sa, socklen_t salen);
void Listen(int fd, int backlog);
Sigfunc* Signal(int signo, Sigfunc *func);

#endif
