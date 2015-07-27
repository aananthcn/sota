#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */

#include "tcpcommon.h"
#include "unixcommon.h"

int Debug = 0;

int main(int argc, char **argv)
{
	int sockfd;
	int c;
	struct sockaddr_in servaddr;
	char *ip;

	if (argc < 2)
		err_quit("usage: sotaclient -s <IPaddress>");

	while ((c = getopt(argc, argv, "s:d")) != -1) {
		switch (c)
		{
			case 's':
				ip = optarg;
				break;
			case 'd':
				Debug = 1;
				break;
			default:
				printf("arg \'-%c\' not supported\n", c);
				break;
		}
	}


	sockfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	Inet_pton(AF_INET, ip, &servaddr.sin_addr);

	Connect(sockfd, (SA *) &servaddr, sizeof(servaddr));

	//str_cli(stdin, sockfd);		/* do it all */
	sota_main(sockfd);

	exit(0);
}
