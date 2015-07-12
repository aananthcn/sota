/* 
 * Author: Aananth C N
 * email: caananth@visteon.com
 *
 * License: GPL v2
 * Date: 10 July 2015
 */

#ifndef SOTAJSON_H
#define SOTAJSON_H

#define JSON_CHUNK_SIZE (4*1024)

typedef enum JSON_OBJ {
	JSON_FILE_OBJ,
	JSON_BUFF_OBJ,
	MAX_JSON_OBJ
}JSON_OBJ_T;



/* APIs to send */
int send_json_file_object(int sockfd, char* filepath);
int send_json_buffer_object(int sockfd, char* buffer, int maxsize);

/* APIs to receive */
int recv_json_file_object(int sockfd, char* filepath);
int recv_json_buffer_object(int sockfd, char* buffer, int maxsize);


#endif
