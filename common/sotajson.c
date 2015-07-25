/* 
 * Author: Aananth C N
 * email: c.n.aananth@gmail.com
 *
 * License: GPL v2
 * Date: 10 July 2015
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include <jansson.h>

#include "unixcommon.h"
#include "sotajson.h"
#include "sotacommon.h"


/*************************************************************************
 * function: sj_load_file
 *
 * This function copies json file to RAM area, do necessary checks and 
 * pass back json_t* (pointer to the RAM area), to the caller
 *
 * arg1: file path of json file
 * arg2: json_t pointer reference
 * 
 * return: positive or negative number
 */
int sj_load_file(char *file, json_t **root)
{
	int flags = 0;

	json_error_t error;

	if(!file) {
		printf("%s(), invalid file passed!\n", __FUNCTION__);
		return -1;
	}

	*root = json_load_file(file, flags, &error);
	if(*root == NULL) {
		printf("error: %s, line %d: %s\n", file,
		       error.line, error.text);
		return -1;
	}

	return 1;
}


/*************************************************************************
 * function: sj_store_file
 *
 * This function stores the data in RAM pointed by json_t* to the storage
 * media path passed as argument
 *
 * arg1: file path
 * arg2: json_t pointer
 * 
 * return: positive or negative number
 */
int sj_store_file(json_t *root, char *file)
{
	int flags;
	char tfile[JSON_NAME_SIZE];
	char chunk[JSON_CHUNK_SIZE+4];

	int ret, fdi, fdo, rcnt, c, i, e;
	int totalcnt = 0, braces = 0;

	flags = JSON_INDENT(8) + JSON_ENSURE_ASCII;
	sprintf(tfile, "%s/store.tmp.json", SessionPath);

	if(!file) {
		printf("%s(), invalid file passed!\n", __FUNCTION__);
		return -1;
	}

	/* dump to a temporary file */
	ret = json_dump_file(root, tfile, flags);
	if(ret < 0)
		return -1;

	/* open files for filtering junk bytes */
	fdi = open(tfile, O_RDONLY);
	if(fdi < 0) {
		printf("could not open %s\n", tfile);
		return -1;
	}
	fdo = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if(fdo < 0) {
		printf("could not open %s\n", file);
		close(fdi);
		return -1;
	}

	do {
		printf(".");
		rcnt = read(fdi, chunk, JSON_CHUNK_SIZE);
		if(rcnt <= 0)
			break;

		for(i = 0, e = 0; i < JSON_CHUNK_SIZE; i++) {
			c = chunk[i];

			if(c == '{')
				braces++;
			else if(c == '}')
				braces--;
			if(braces < 0)
				goto exit_fail;

			if((c == '}') && (braces == 0)) {
				chunk[i+1] = '\n';
				chunk[i+2] = '\0';
				e += 2;
				break;
			}
		}
		totalcnt += (rcnt+e);
		write(fdo, chunk, rcnt+e);
	} while (1);

	ftruncate(fdo, totalcnt+1);
	close(fdi);
	close(fdo);

	return ret;

exit_fail:
	close(fdi);
	close(fdo);
	printf("%s(): Invalid json file passed\n", __FUNCTION__);
	return -1;

}



/*************************************************************************
 * function: sj_get_int
 *
 * This function gets the integer value from the json object
 *
 * return: positive or negative number
 */
int sj_get_int(json_t *root, char *name, int *value)
{
	json_t *obj;

	obj = json_object_get(root, name);
	if(obj == NULL) {
		*value = 0;
		return -1;
	}

	*value = json_integer_value(obj);
	return 0;
}



/*************************************************************************
 * function: sj_get_string
 *
 * This function gets the string value from the json object
 *
 * return: positive or negative number
 */
int sj_get_string(json_t *root, char *name, char *value)
{
	json_t *obj;

	if(!json_is_object(root)) {
		printf("%s(): invalid json arg passed\n", __FUNCTION__);
		return -1;
	}

	obj = json_object_get(root, name);
	if(obj == NULL) {
		*value = '\0';
		return -1;
	}

	strcpy(value, json_string_value(obj));
	return 0;
}



/*************************************************************************
 * function: sj_set_int
 *
 * This function sets the integer value to the json object
 *
 * return: positive or negative number
 */
int sj_set_int(json_t *root, char *name, int value)
{
	json_t *obj;

	obj = json_object_get(root, name);
	if(obj == NULL) {
		return -1;
	}

	return json_integer_set(obj, value);
}



/*************************************************************************
 * function: sj_set_string
 *
 * This function gets the string value to the json object
 *
 * return: positive or negative number
 */
int sj_set_string(json_t *root, char *name, char *value)
{
	json_t *obj;

	obj = json_object_get(root, name);
	if(obj == NULL) {
		return -1;
	}

	return json_string_set(obj, value);
}


int sj_add_int(json_t **root, char *name, int value)
{
	json_t *new;

	new = json_pack("{\nsi\n}", name, value);
	if(new  == NULL)
		return -1;

	return json_object_update(*root, new);
}

int sj_add_string(json_t **root, char *name, char *value)
{
	json_t *new;

	new = json_pack("{\nss\n}", name, value);
	if(new  == NULL)
		return -1;

	return json_object_update(*root, new);
}

/* helper function to populate header required for sotajson protocol */
int sj_create_header(json_t **root, char *name, int size)
{
	*root = json_object();

	if((size < 1024) || (name == NULL)) {
		printf("%s, check arguments\n", __FUNCTION__);
		return -1;
	}

	if(0 > sj_add_string(root, "msg_name", name)) {
		printf("%s, failed to add name\n", __FUNCTION__);
		return -1;
	}

	if(0 > sj_add_int(root, "msg_size", size)) {
		printf("%s, failed to add size\n", __FUNCTION__);
		return -1;
	}

	return 0;
}

/*************************************************************************
 * Function: validate_json_file_size
 *
 * This function checks if the file size is not more than 1 MB and also 
 * rounds the size to JSON_CHUNK_SIZE value.
 *
 * arg1: original size from json file
 * 
 * return: rounded or adjusted files size.
 */
int validate_json_file_size(int isize)
{
	int osize;
	int limit = (1024 * 1024);
	int jsize = JSON_CHUNK_SIZE;

	if(isize > limit)
		osize = limit;
	else if(isize <= 0)
		osize = -1;
	else
		osize = ((int)((isize-1)/jsize)+1) * jsize;

	return osize;
}

/*************************************************************************
 * Function: verify_json_get_size
 *
 * This function checks for 2 critical names present in the buffer, they 
 * are: "msg_name" and "msg_size". If both are not found, then the function
 * will return negative, else positive number.
 *
 * arg1: buffer pointer
 * arg2: buffer length
 * 
 * return: if success it returnes the msg_size from the json object, else 
 * a negative number.
 */
int verify_json_get_size(char *buffer, int len)
{
	int flags = 0;
	int ret = -1;
	int size;
	int intchk, strchk;
	char *plocalb;

	json_t *root, *jmsgn, *jsize;
	json_error_t error;

	plocalb = malloc(len+4);
	if(plocalb == NULL) {
		printf("%s(), buffer allocation failure\n", __FUNCTION__);
		return -1;
	}

	/* copy to local buffer to modify it during verification */
	memcpy(plocalb, buffer, len);
	plocalb[len+1] = '\0';

	root = json_loads(plocalb, flags, &error);

	if(root != NULL) {
		jmsgn = json_object_get(root, "msg_name");
		jsize = json_object_get(root, "msg_size");

		if((jmsgn == NULL) || (jsize == NULL))
			printf("%s(): Invalid SOTA JSON buffer\n",
			      __FUNCTION__);
		else {
			/* check if header objects are valid */
			intchk = json_is_integer(jsize);
			strchk = json_is_string(jmsgn);
			if((intchk) || (strchk)) {
				printf("MSG: %s\n", json_string_value(jmsgn));
				size = json_integer_value(jsize);
				ret = validate_json_file_size(size);
			}
			else {
				printf("%s(): Invalid JSON size format\n",
				       __FUNCTION__);
				goto exit_this;
			}
		}
	}
	else {
		printf("error: on line %d: %s\n", error.line, error.text);
		printf("buffer:\n%s\n", buffer);
	}

exit_this:
	free(plocalb);

	return ret;
}



/*************************************************************************
 * Function: sj_send_file_object
 *
 * This function sends the json file passed as argument to the remote 
 * machine. The function sends bytes in JSON_CHUNK_SIZE chunks
 *
 * arg1: file descriptor of the socket
 * arg2: filename incl. path that will be sent to the remote machine
 *
 * Note: please pass file path that corresponds to RAM area. Eg. /tmp/...
 *
 * return   : number of bytes sent or negative value 
 */
int sj_send_file_object(int sockfd, char* filepath)
{
	int fd, rcnt, wcnt;
	int totalcnt = 0;
	int json_check_done = 0;
	int jobj_size, chunksize;
	char chunk[JSON_CHUNK_SIZE];

	if((filepath == NULL) || (sockfd <= 0)) {
		printf("%s() called with incorrect arguments\n", __FUNCTION__);
		return -1;
	}

	fd = open(filepath, O_RDONLY);
	if(fd < 0) {
		printf("%s(): Unable to open file %s for reading\n",
		       __FUNCTION__, filepath);
		return -1;
	}

	/* minimum size both parties need to transact for the first time */
	chunksize = JSON_CHUNK_SIZE;

	do {
		/* read a chunk from the input file */
		rcnt = read(fd, chunk, chunksize);
		if(rcnt < 0) {
			if(errno == EINTR)
				continue;
			printf("Read error in %s()\n", __FUNCTION__);
			totalcnt = -1 * (totalcnt + 1);
			break;
		}

		/* check for end of file */
		if(rcnt == 0) {
			break;
		}

		if(!json_check_done) {
			jobj_size = verify_json_get_size(chunk, chunksize);
			if(jobj_size < 0)
				continue; /* ignore this chunk */
			else
				json_check_done = 1;
		}

		/* write the chunk to the socket connection */
		wcnt = write(sockfd, chunk, chunksize);
		if(wcnt < 0) {
			if(errno == EINTR)
				continue;
			printf("write error %s in %s()\n", strerror(errno),
			       __FUNCTION__);
			totalcnt = -1 * (totalcnt + 1);
			break;
		}
		totalcnt += wcnt;

		/* compute the balance bytes to be sent */
		jobj_size -= wcnt;
		if(jobj_size <= 0)
			break; /* we are done!! */

		/* compute the number of bytes to be sent next */
		if(jobj_size > JSON_CHUNK_SIZE)
			chunksize = JSON_CHUNK_SIZE;
		else
			chunksize = jobj_size;
	} while (1);

	close(fd);
	return totalcnt;
}



/*************************************************************************
 * Function: sj_recv_file_object
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
int sj_recv_file_object(int sockfd, char *filepath)
{
	int fd, rcnt;
	int totalcnt = 0, filesize = 0;
	int json_check_done = 0;
	int jobj_size, chunksize;
	int  eof_found = 0, eof_size = 0;
	char chunk[JSON_CHUNK_SIZE];
	char *eof;

	if((filepath == NULL) || (sockfd <= 0)) {
		printf("%s() called with incorrect arguments\n", __FUNCTION__);
		return -1;
	}

	fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if(fd < 0) {
		printf("Unable to open file %s for writing\n", filepath);
		printf("error: %s\n", strerror(errno));
		return -1;
	}

	/* minimum size both parties need to transact for the first time */
	chunksize = JSON_CHUNK_SIZE;

	do {
		/* read a chunk from socket connection */
		rcnt = read(sockfd, chunk, chunksize);
		if(rcnt < 0) {
			if(errno == EINTR)
				continue;
			printf("Read error \"%s\" in %s()\n", strerror(errno),
			       __FUNCTION__);
			totalcnt = -1 * (totalcnt + 1);
			break;
		}

		/* check for end of file */
		if(rcnt == 0) {
			break;
		}

		if(!json_check_done) {
			jobj_size = verify_json_get_size(chunk, chunksize);
			if(jobj_size < 0)
				continue; /* ignore this chunk */
			else
				json_check_done = 1;
		}
		totalcnt += rcnt;
		write(fd, chunk, rcnt);

		/* need to truncate the file after eof, else json error */
		if(!eof_found) {
			eof = memchr(chunk, '\0', rcnt);
			if(eof) {
				eof_size += (eof - chunk);
				eof_found = 1;
			}
			else
				eof_size += rcnt;
		}

		/* compute the balance bytes to be received */
		jobj_size -= rcnt;
		if(jobj_size <= 0)
			break; /* we are done!! */

		/* compute the number of bytes to be received next */
		if(jobj_size > JSON_CHUNK_SIZE)
			chunksize = JSON_CHUNK_SIZE;
		else
			chunksize = jobj_size;
	} while (1);

	ftruncate(fd, eof_size);
	close(fd);

	return totalcnt;
}
