#include <strings.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <signal.h>
#include <stdlib.h>

#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "tcpcommon.h"
#include "unixcommon.h"
#include "sotaserver.h"

unsigned long Sessions;
int Connections;
int Debug = 0;


void ssl_show_certificate(SSL* ssl)
{
	X509 *cert;
	char *line;
	long res;

	/* get the server's certificate */
	cert = SSL_get_peer_certificate(ssl);

	if(cert != NULL) {
		printf("Client certificate:\n");
		line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
		printf("\tSubject: %s\n", line);
		free(line);
		line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
		printf("\tIssuer: %s\n", line);
		free(line);

		X509_free(cert);
	}
	else {
		printf("OpenSSL: No certificates from client.\n");
		res = SSL_get_verify_result(ssl);
		if(X509_V_OK != res) {
			printf("Certificate error code: %lu\n", res);
			ERR_print_errors_fp(stderr);
		}
	}
	printf("\n");
}


int load_certificates(SSL_CTX *ctx)
{
	int fec, fek;

	char CertFile[] = "security/cacert.pem";
	char KeyFile[] = "security/private.pem";

	fec = access(CertFile, F_OK);
	fek = access(KeyFile, F_OK);

	if((fec != 0) || (fek != 0)) {
		printf("Unable access certificate files\n");
		return -1;
	}

	/* set the local certificate from CertFile */
	if(0 >= SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM)) {
		ERR_print_errors_fp(stderr);
		printf("Error loading file \"%s\"\n", CertFile);
		return -1;
	}

	/* set the private key from KeyFile */
	if(0 >= SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM)) {
		ERR_print_errors_fp(stderr);
		printf("Error loading file \"%s\"\n", KeyFile);
		return -1;
	}

	/* verify private key */
	if(!SSL_CTX_check_private_key(ctx)) {
		ERR_print_errors_fp(stderr);
		return -1;
	}

	return 0;
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
	method = SSLv3_server_method();

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
	int			listenfd, connfd, c;
	int			en = 1;
	pid_t			childpid;
	socklen_t		clilen;
	char			*cfgfile;
	struct sockaddr_in	cliaddr, servaddr;
	void sig_chld(int);

	SSL_CTX			*ssl_ctx;
	SSL			*ssl;

	if(argc < 3) {
		printf("usage: sotaserver -i <cfgfile>\n");
		return -1;
	}

	while ((c = getopt(argc, argv, "i:d")) != -1) {
		switch (c) {
		case 'i':
			cfgfile = optarg;
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

	if(0 > load_certificates(ssl_ctx))
		return -1;

	listenfd = Socket(AF_INET, SOCK_STREAM, 0);
	if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &en, sizeof(int)) < 0)
		printf("setsockopt(SO_REUSEADDR) failed\n");

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(SERV_PORT);

	Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

	Listen(listenfd, LISTENQ); /* check LISTENQ to increase more clients */

	Signal(SIGCHLD, sig_chld);
	printf("Server initialization complete, waiting for clients...\n");

	/* initial critical paths for the main thread */
	strcpy(SessionPath, "/tmp/sota");
	create_dir(SessionPath);

	for( ; ; ) {
		clilen = sizeof(cliaddr);
		if((connfd = accept(listenfd, (SA *) &cliaddr, &clilen)) < 0) {
			if (errno == EINTR)
				continue;
			else
				err_sys("accept error");
		}

		Sessions++;
		if((childpid = Fork()) == 0) {	/* child process */
			Connections++;
			Close(listenfd);	/* close listening socket */
			/* convert to a secure socket */
			ssl = SSL_new(ssl_ctx);
			SSL_set_fd(ssl, connfd);
			if(-1 == SSL_accept(ssl))
				ERR_print_errors_fp(stderr);

			/* print certificates for server admin */
			printf("\n\nStart of SOTA session %lu\n", Sessions);
			ssl_show_certificate(ssl);

			/* do the main job */
			sota_main(ssl, cfgfile);
			printf("SOTA Session Ended!\n\t");

			/* close connection and clean up */
			connfd = SSL_get_fd(ssl);
			SSL_free(ssl);
			Connections--;
			exit(0);
		}
		Close(connfd);	/* parent closes connected socket */

#if 0
		/* check if cache memory exceeded limits */
		if(monitor_cache_dir()) {
			/* wait for current connections to close */
			while(Connections)
				usleep(1000);
			trim_cache_dir();
		}
#endif

	} /* for loop */
	SSL_CTX_free(ssl_ctx);
}
