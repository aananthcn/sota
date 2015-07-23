/* 
 * Author: Aananth C N
 * email: c.n.aananth@gmail.com
 *
 * License: GPL v2
 * Date: 23 July 2015
 */

#ifndef SOTABDATA_H
#define SOTABDATA_H


#define BDATA_CHUNK_SIZE (4*1024)

/* APIs to send data to remote node */
int sb_send_file_object(int sockfd, char *filepath, int total);

/* APIs to receive data from remote node */
int sb_recv_file_object(int sockfd, char *filepath, int total);





#endif
