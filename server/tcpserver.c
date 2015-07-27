#include <strings.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <signal.h>
#include <stdlib.h>

#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */

#include "tcpcommon.h"
#include "unixcommon.h"
#include "sotadb.h"

unsigned long Sessions;
int Debug = 0;

int main(int argc, char **argv)
{
	int		   listenfd, connfd, c;
	pid_t		   childpid;
	socklen_t	   clilen;
	struct sockaddr_in cliaddr, servaddr;
	void sig_chld(int);

	while ((c = getopt(argc, argv, "s:d")) != -1) {
		switch (c)
		{
			case 'd':
				Debug = 1;
				break;
			default:
				printf("arg \'-%c\' not supported\n", c);
				break;
		}
	}

	if(0 > db_init()) {
		return -1;
	}

	listenfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(SERV_PORT);

	Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

	Listen(listenfd, LISTENQ);

	Signal(SIGCHLD, sig_chld);

	for( ; ; ) {
		clilen = sizeof(cliaddr);
		if((connfd = accept(listenfd, (SA *) &cliaddr, &clilen)) < 0) {
			if (errno == EINTR)
				continue;		/* back to for() */
			else
				err_sys("accept error");
		}

		Sessions++;
		if((childpid = Fork()) == 0) {	/* child process */
			Close(listenfd);	/* close listening socket */
			sota_main(connfd);
			exit(0);
		}
		Close(connfd);			/* parent closes connected socket */
	}
}
