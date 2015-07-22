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
#define JSON_NAME_SIZE (256)

typedef enum JSON_OBJ {
	JSON_FILE_OBJ,
	JSON_BUFF_OBJ,
	MAX_JSON_OBJ
}JSON_OBJ_T;



/* APIs to send data to remote node */
int sj_send_file_object(int sockfd, char *filepath);

/* APIs to receive data from remote node */
int sj_recv_file_object(int sockfd, char *filepath);

/* other useful APIs */
int sj_load_file(char *file, json_t **root);
int sj_store_file(json_t *root, char *file);
int sj_get_int(json_t *root, char *name, int *value);
int sj_get_string(json_t *root, char *name, char *value);
int sj_set_int(json_t *root, char *name, int value);
int sj_set_string(json_t *root, char *name, char *value);
int sj_add_int(json_t **root, char *name, int value);
int sj_add_string(json_t **root, char *name, char *value);


#endif
