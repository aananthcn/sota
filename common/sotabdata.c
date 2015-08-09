/* 
 * Author: Aananth C N
 * email: c.n.aananth@gmail.com
 *
 * License: GPL v2
 * Date: 23 July 2015
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "unixcommon.h"
#include "sotabdata.h"
#include "sotacommon.h"



/*************************************************************************
 * Function: sb_send_file_object
 *
 * This function sends the json file passed as argument to the remote 
 * machine. The function sends bytes in BDATA_CHUNK_SIZE chunks
 *
 * arg1: file descriptor of the socket
 * arg2: filename incl. path that will be sent to the remote machine
 *
 * Note: please pass file path that corresponds to RAM area. Eg. /tmp/...
 *
 * return   : number of bytes sent or negative value 
 */
int sb_send_file_object(SSL *conn, char* filepath, int total)
{
	int fd, rcnt, wcnt;
	int bal_bytes = total;
	int chunksize;
	char chunk[BDATA_CHUNK_SIZE];

	if((filepath == NULL) || (conn == NULL) || (total <= 0)) {
		printf("%s() called with incorrect arguments\n", __FUNCTION__);
		return -1;
	}

	fd = open(filepath, O_RDONLY);
	if(fd < 0) {
		printf("%s(): Unable to open file %s for reading\n",
		       __FUNCTION__, filepath);
		return -1;
	}
	printf("   ==> %d bytes data\n", total);

	do {
		/* compute the number of bytes to be sent */
		if(bal_bytes > BDATA_CHUNK_SIZE)
			chunksize = BDATA_CHUNK_SIZE;
		else
			chunksize = bal_bytes;

		/* read a chunk from the input file */
		rcnt = read(fd, chunk, chunksize);
		if(rcnt < 0) {
			if(errno == EINTR)
				continue;
			printf("Read error in %s()\n", __FUNCTION__);
			bal_bytes = -1 * (bal_bytes + 1);
			break;
		}

		/* check for end of file */
		if(rcnt == 0) {
			break;
		}

		/* write the chunk to the socket connection */
		wcnt = SSL_write(conn, chunk, rcnt);
		if(wcnt < 0) {
			if(errno == EINTR)
				continue;
			printf("write error %s in %s()\n", strerror(errno),
			       __FUNCTION__);
			bal_bytes = -1 * (bal_bytes + 1);
			break;
		}
		bal_bytes -= wcnt;
	} while (bal_bytes > 0);

	close(fd);

	return (total - bal_bytes);
}



/*************************************************************************
 * Function: sb_recv_file_object
 *
 * This function populates the incoming bytes from the remote machine into
 * the file path provided in the 2nd argument of this function.
 *
 * arg1: file descriptor of the socket
 * arg2: filename incl. path where this function copies the received data
 *
 * Note: please pass file path that corresponds to RAM area. Eg. /tmp/...
 *
 * return   : number of bytes received or negative value 
 */
int sb_recv_file_object(SSL *conn, char *filepath, int total)
{
	int  fd, rcnt;
	int  bal_bytes = total;
	int  filesize = 0;
	int  chunksize;
	int  eof_found = 0, eof_size = 0;
	char chunk[BDATA_CHUNK_SIZE];
	char *eof;

	if((filepath == NULL) || (conn == NULL) || (total <= 0)) {
		printf("%s() called with incorrect arguments\n", __FUNCTION__);
		return -1;
	}

	fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if(fd < 0) {
		printf("Unable to open file %s for writing\n", filepath);
		printf("error: %s\n", strerror(errno));
		return -1;
	}
	printf("   <== %d bytes data\n", total);

	do {
		/* compute the number of bytes to be received */
		if(bal_bytes > BDATA_CHUNK_SIZE)
			chunksize = BDATA_CHUNK_SIZE;
		else
			chunksize = bal_bytes;

		/* read a chunk from socket connection */
		rcnt = SSL_read(conn, chunk, chunksize);
		if(rcnt < 0) {
			if(errno == EINTR)
				continue;
			printf("Read error in %s()\n", __FUNCTION__);
			bal_bytes = -1 * (bal_bytes + 1);
			break;
		}

		/* check for end of file */
		if(rcnt == 0) {
			break;
		}

		write(fd, chunk, rcnt);
		bal_bytes -= rcnt;

	} while (bal_bytes > 0);

	close(fd);

	return (total - bal_bytes);
}
