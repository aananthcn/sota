#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "tcpcommon.h"
#include "unixcommon.h"

int Debug = 0;




void ssl_show_certificate(SSL* ssl)
{
	X509 *cert;
	char *line;
	long res;

	/* get the server's certificate */
	cert = SSL_get_peer_certificate(ssl);

	printf("\n\n");
	if(cert != NULL) {
		printf("Server certificate:\n");
		line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
		printf("\tSubject: %s\n", line);
		free(line);
		line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
		printf("\tIssuer: %s\n", line);
		free(line);

		X509_free(cert);
	}
	else {
		printf("OpenSSL: No certificates from server.\n");
		res = SSL_get_verify_result(ssl);
		if(X509_V_OK != res) {
			printf("Certificate error code: %lu\n", res);
			ERR_print_errors_fp(stderr);
		}
	}
	printf("\n");
}

SSL_CTX* ssl_init_context(void)
{
	const SSL_METHOD *method;
	SSL_CTX *ctx;

	SSL_library_init();

	/* load & register all cryptos, etc. */
	OpenSSL_add_all_algorithms();

	/* load all error messages */
	SSL_load_error_strings();

	/* create new server-method instance */
	method = SSLv3_client_method();

	/* create new context from method */
	ctx = SSL_CTX_new(method);
	if(ctx == NULL) {
		ERR_print_errors_fp(stderr);
		printf("Unable to create new SSL Context\n");
	}

	return ctx;
}



int main(int argc, char **argv)
{
	int sockfd;
	int c;
	struct sockaddr_in servaddr;
	char *ip;

	SSL_CTX *ssl_ctx;
	SSL *ssl;

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

	ssl_ctx = ssl_init_context();
	if(ssl_ctx == NULL)
		return -1;

	sockfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	Inet_pton(AF_INET, ip, &servaddr.sin_addr);

	Connect(sockfd, (SA *) &servaddr, sizeof(servaddr));

	/* convert to a secure socket */
	ssl = SSL_new(ssl_ctx);
	SSL_set_fd(ssl, sockfd);

	/* perform the connection */
	if(SSL_connect(ssl) == -1)
		ERR_print_errors_fp(stderr);
	else {
		printf("\nStart of SOTA session");
		ssl_show_certificate(ssl);
		sota_main(ssl); /* client's main */
		printf("SOTA Session Ended!\n\n");
	}

	/* clean up */
	SSL_free(ssl);
	SSL_CTX_free(ssl_ctx);

	exit(0);
}
