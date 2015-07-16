/* 
 * Author: Aananth C N
 * email: caananth@visteon.com
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


/*************************************************************************
 * function: load_json_file
 *
 * This function copies json file to RAM area, do necessary checks and 
 * pass back json_t* (pointer to the RAM area), to the caller
 *
 * arg1: file path of json file
 * arg2: json_t pointer reference
 * 
 * return: positive or negative number
 */
int load_json_file(char *file, json_t **root)
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

	return 0;
}


/*************************************************************************
 * function: store_json_file
 *
 * This function stores the data in RAM pointed by json_t* to the storage
 * media path passed as argument
 *
 * arg1: file path
 * arg2: json_t pointer
 * 
 * return: positive or negative number
 */
int store_json_file(json_t *root, char *file)
{
	int flags = 0;

	if(!file) {
		printf("%s(), invalid file passed!\n", __FUNCTION__);
		return -1;
	}

	return json_dump_file(root, file, flags);
}



/*************************************************************************
 * function: get_json_int
 *
 * This function gets the integer value from the json object
 *
 * return: positive or negative number
 */
int get_json_int(json_t *root, char *name, int *value)
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
 * function: get_json_string
 *
 * This function gets the string value from the json object
 *
 * return: positive or negative number
 */
int get_json_string(json_t *root, char *name, char *value)
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
 * function: set_json_int
 *
 * This function sets the integer value to the json object
 *
 * return: positive or negative number
 */
int set_json_int(json_t *root, char *name, int value)
{
	json_t *obj;

	obj = json_object_get(root, name);
	if(obj == NULL) {
		return -1;
	}

	return json_integer_set(obj, value);
}



/*************************************************************************
 * function: set_json_string
 *
 * This function gets the string value to the json object
 *
 * return: positive or negative number
 */
int set_json_string(json_t *root, char *name, char *value)
{
	json_t *obj;

	obj = json_object_get(root, name);
	if(obj == NULL) {
		return -1;
	}

	return json_string_set(obj, value);
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
 * arg3: mst_name value from json file will be copied back to the caller
 *       buffer. Note: The caller has to allocate memory for this.
 * 
 * return: if success it returnes the msg_size from the json object, else 
 * a negative number.
 */
int verify_json_get_size(char *buffer, int len, char *msgname)
{
	int flags = 0;
	int ret = -1;
	int size;
	char *plocalb;

	json_t *root, *jmsgn, *jsize;
	json_error_t error;

	plocalb = malloc(len+4);
	if(plocalb == NULL) {
		printf("%s(), buffer allocation failure\n", __FUNCTION__);
		return -1;
	}

	/* copy to local buffer so that buffer is modified and processed */
	memcpy(plocalb, buffer, len);
	plocalb[len+1] = '\0';

	root = json_loads(plocalb, flags, &error);

	if(root != NULL) {
		jmsgn = json_object_get(root, "msg_name");
		jsize = json_object_get(root, "msg_size");

		if((jmsgn == NULL) || (jsize == NULL))
			printf("Invalid SOTA JSON buffer\n");
		else {
			if(json_is_integer(jsize)) {
				/* valid sota-json object */
				size = json_integer_value(jsize);
				ret = validate_json_file_size(size);

			}
			else {
				printf("Invalid JSON size format\n");
				goto exit_this;
			}

			/* copy back the msg_name from the json object */
			if((msgname) && (json_is_string(jmsgn))) {
				strncpy(msgname, json_string_value(jmsgn),
					JSON_NAME_SIZE);
				msgname[JSON_NAME_SIZE] = '\0';
			}
			else {
				printf("Invalid JSON name format\n");
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
 * Function: send_json_file_object
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
int send_json_file_object(int sockfd, char* filepath)
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
		printf("Unable to open file %s for reading\n", filepath);
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
			jobj_size = verify_json_get_size(chunk, chunksize,
							 NULL);
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
 * Function: send_json_buffer_object
 * 
 * This function sends the bytes stored in buffer to the remote machine.
 * The buffer location is provided in the 2nd argument of this function.
 *
 * arg1: file descriptor of the socket
 * arg2: user memory buffer, expected to be larger than JSON_CHUNK_SIZE
 * arg3: maximum size of user buffer
 *
 * return   : number of bytes received or negative value 
 */
int send_json_buffer_object(int sockfd, char* buffer, int maxsize)
{
	int wcnt, rcnt, jobj_size, chunksize;
	int totalcnt = 0;
	char *pchunk;

	if((maxsize <= 0) || (buffer == NULL) || (sockfd <= 0)) {
		printf("%s() called with incorrect arguments\n", __FUNCTION__);
		return -1;
	}


	jobj_size = verify_json_get_size(buffer, maxsize, NULL);
	if(jobj_size < 0) {
		printf("%s(), invalid json buffer\n", __FUNCTION__);
		return -1;
	}

	/* minimum size both parties need to transact for the first time */
	chunksize = JSON_CHUNK_SIZE;
	pchunk = buffer;

	do {
		/* write a chunk to the socket connection */
		wcnt = write(sockfd, pchunk, chunksize);
		if(wcnt < 0) {
			if(errno == EINTR)
				continue;
			printf("Write error in %s()\n", __FUNCTION__);
			totalcnt = (totalcnt+1) * (-1);
			break;
		}

		if(rcnt == 0) {
			continue;
		}
		totalcnt += wcnt;

		/* compute the balance bytes to be sent */
		jobj_size -= wcnt;
		if(jobj_size <= 0)
			break; /* we have sent all bytes */

		/* compute the chunk size for the next transmission */
		if(jobj_size > JSON_CHUNK_SIZE)
			chunksize = JSON_CHUNK_SIZE;
		else
			chunksize = jobj_size;

		/* check if the buffer has this chunk size bytes */
		if(maxsize > (totalcnt + chunksize))
			pchunk = buffer + totalcnt;
		else
			break; /* not enough space to send a chunk */
	} while (1);

	return totalcnt;

}



/*************************************************************************
 * Function: recv_json_file_object
 *
 * This function populates the incoming bytes from the remote machine into
 * the file path provided in the 2nd argument of this function.
 *
 * arg1: file descriptor of the socket
 * arg2: filename incl. path where this function copies the received data
 * arg3: msg_type value of incoming json will be passed back to the caller
 *
 * Note: please pass file path that corresponds to RAM area. Eg. /tmp/...
 *
 * return   : number of bytes received or negative value 
 */
int recv_json_file_object(int sockfd, char *filepath, char *msgname)
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
			printf("Read error in %s()\n", __FUNCTION__);
			totalcnt = -1 * (totalcnt + 1);
			break;
		}

		/* check for end of file */
		if(rcnt == 0) {
			break;
		}

		if(!json_check_done) {
			jobj_size = verify_json_get_size(chunk, chunksize,
							 msgname);
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
				printf("found eof at %d, rcnt = %d\n", eof_size, rcnt);
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



/*************************************************************************
 * Function: recv_json_buffer_object
 * 
 * This function populates the incoming bytes from the remote machine into
 * the buffer location provided in the 2nd argument of this function.
 *
 * arg1: file descriptor of the socket
 * arg2: user memory buffer, expected to be larger than JSON_CHUNK_SIZE
 * arg3: maximum size of user buffer
 *
 * return   : number of bytes received or negative value 
 */
int recv_json_buffer_object(int sockfd, char* buffer, int maxsize)
{
	int rcnt;
	int totalcnt = 0;
	int json_check_done = 0;
	int jobj_size, chunksize;
	char *pchunk;

	if((maxsize <= 0) || (buffer == NULL) || (sockfd <= 0)) {
		printf("%s() called with incorrect arguments\n", __FUNCTION__);
		return -1;
	}

	if(maxsize < JSON_CHUNK_SIZE) {
		printf("%s(), user buffer size is less than minumum\n",
		       __FUNCTION__);
		return -1;
	}

	/* minimum size both parties need to transact for the first time */
	chunksize = JSON_CHUNK_SIZE;
	pchunk = buffer;

	do {
		/* read a chunk from socket connection */
		rcnt = read(sockfd, pchunk, chunksize);
		if(rcnt < 0) {
			if(errno == EINTR)
				continue;
			printf("Read error in %s()\n", __FUNCTION__);
			totalcnt = (totalcnt+1) * (-1);
			break;
		}

		/* check for end of file */
		if(rcnt == 0) {
			break;
		}

		/* check if the chunk has json objects */
		if(!json_check_done) {
			jobj_size = verify_json_get_size(pchunk,
							 chunksize, NULL);
			if(jobj_size < 0)
				continue; /* ignore this chunk */
			else
				json_check_done = 1;
		}
		totalcnt += rcnt;

		/* compute the balance bytes to be received */
		jobj_size -= rcnt;
		if(jobj_size <= 0)
			break; /* we are done!! */

		/* compute the chunk size for the next reception */
		if(jobj_size > JSON_CHUNK_SIZE)
			chunksize = JSON_CHUNK_SIZE;
		else
			chunksize = jobj_size;

		/* check if we can receive another chunk */
		if(maxsize > (totalcnt + chunksize))
			pchunk = buffer + totalcnt;
		else
			break; /* not enough space to receive */
	} while (1);


	return totalcnt;
}
