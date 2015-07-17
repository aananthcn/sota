/* 
 * Author: Aananth C N
 * email: caananth@visteon.com
 *
 * License: GPL v2
 * Date: 10 July 2015
 */

#ifndef SOTAJSON_H
#define SOTAJSON_H

#include <jansson.h>

#define JSON_CHUNK_SIZE (4*1024)
#define JSON_NAME_SIZE (128)

typedef enum JSON_OBJ {
	JSON_FILE_OBJ,
	JSON_BUFF_OBJ,
	MAX_JSON_OBJ
}JSON_OBJ_T;



/* APIs to send data to remote node */
int send_json_file_object(int sockfd, char *filepath);

/* APIs to receive data from remote node */
int recv_json_file_object(int sockfd, char *filepath);

/* other useful APIs */
int load_json_file(char *file, json_t **root);
int store_json_file(json_t *root, char *file);
int get_json_int(json_t *root, char *name, int *value);
int get_json_string(json_t *root, char *name, char *value);
int set_json_int(json_t *root, char *name, int value);
int set_json_string(json_t *root, char *name, char *value);
int add_json_int(json_t **root, char *name, int value);
int add_json_string(json_t **root, char *name, char *value);


#endif
