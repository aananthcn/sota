#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/stat.h>

#include "unixcommon.h"

int daemon_proc;


/*************************************************************************
 * Function: tolower_str
 *
 * This function changes all upper case characters to lower case.
 */
int print(const char *format, ...)
{
	va_list arg;
	int done;

#if defined (ANDROID) || defined (__ANDROID__)
	#include <android/log.h>
	char msg[4*1024];

	va_start(arg, format);
	vsprintf(msg, format, arg);
	perror(msg);

	//__android_log_print(ANDROID_LOG_INFO, "MOTA", __VA_ARGS__);
	__android_log_print(ANDROID_LOG_INFO, "MOTA", msg, arg);

#else
	va_start(arg, format);
	done = vfprintf(stdout, format, arg);
#endif

	va_end (arg);

	return done;
}

/*************************************************************************
 * Function: tolower_str
 *
 * This function changes all upper case characters to lower case.
 */
int tolower_str(char *str)
{
	int i, len;

	if(str == NULL) {
		printf("%s(), invalid argument passed\n", __FUNCTION__);
		return -1;
	}

	len = strlen(str);
	for(i = 0; i < len; i++) {
		str[i] = tolower(str[i]);
	}

	return 0;
}


/*************************************************************************
 * Function: humanstr_to_int
 *
 * This function converts string to int and intelligent enought to convert 
 * G, M and K texts as well
 */
int humanstr_to_int(char *str, int *ref)
{
	int len, i;
	int value = 0;

	if((str == NULL) || (ref == NULL))
		return -1;

	len = strlen(str);
	for(i=0; i < len; i++) {
		if((str[i] > '9') || (str[i] < '0')) {
			switch (str[i]) {
			case 'G':
				value *= (1024*1024*1024);
				break;
			case 'M':
				value *= (1024*1024);
				break;
			case 'K':
				value *= (1024);
				break;
			default:
				break;
			}
			break;
		}
		else {
			value = value * 10 + str[i] - '0';
		}
	}

	*ref = value;
	return 0;
}

/*************************************************************************
 * Function: cut_sha256sum_fromfile
 *
 * This function receives a file containing sha256sum output string as 
 * first argument and it extracts the first part of the output string into
 * the second argument passed. 
 *
 * arg1: file containing sha256sum output string
 * arg2: buffer reference point to copy the result
 * arg3: max size of the buffer passed
 *
 * Returns negative if any failure found
 */
int cut_sha256sum_fromfile(char *file, char *value, int valsize)
{
	FILE *fp;
	char *sp;

	if((file == NULL) || (value == NULL)) {
		printf("%s() - invalid parameter passed\n", __FUNCTION__);
		return -1;
	}
	fp = fopen(file, "r");
	if(fp == 0) {
		printf("Can't open %s\n", file);
		return -1;
	}
	fgets(value, valsize, fp);
	fclose(fp);

	/* retain sha256 sum string alone */
	sp = memchr(value, ' ', valsize);
	*sp = '\0';

	return 0;
}


/*************************************************************************
 * Function: get_filelines
 *
 * This function finds the number of lines in a file and returns it. 
 *
 * arg1: file path
 *
 * Returns negative if any failure found else the total lines
 */
int get_filelines(char *file)
{
	int c, lines = 0;
	FILE *fp;

	fp = fopen(file, "r");
	if(fp == NULL) {
		printf("Can't open \"%s\"\n", file);
		return -1;
	}

	do {
		c = fgetc(fp);
		if(c == '\n')
			lines++;
	} while (c != EOF);


	fclose(fp);

	return lines;
}


/*************************************************************************
 * Function: get_filesize
 *
 * This function finds the number of bytes in a file and returns it. 
 *
 * arg1: file path
 *
 * Returns negative if any failure found else the total bytes
 */
int get_filesize(char *file)
{
	int size;
	FILE *fp;

	fp = fopen(file, "r");
	if(fp == NULL) {
		printf("Can't open \"%s\"\n", file);
		return -1;
	}
	fseek(fp, 0L, SEEK_END);
	size = ftell(fp);
	fclose(fp);

	return size;
}


/*************************************************************************
 * Function: get_filename
 *
 * This function finds the name of the file from the path
 *
 * arg1: file path
 *
 * Returns null if any failure found else the file name
 */
char* get_filename(char *path)
{
	int i;

	if(path == NULL)
		return path;

	i = strlen(path);
	for(; i > 0; i--)
		if(path[i] == '/') {
			i++;
			break;
		}

	return (path+i);
}



int create_dir(char *dir)
{
	struct stat st = {0};

	/* create directory for storing temp files */
	if(stat(dir, &st) == -1) {
		mkdir(dir, 0777);
	}
	else
		return -1;

	return 0;
}


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
		syslog(level, "%s", buf);
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

