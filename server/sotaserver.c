/* 
 * Author: Aananth C N
 * email: caananth@visteon.com
 *
 * License: GPL v2
 * Date: 12 July 2015
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "sotajson.h"
#include "sotadb.h"

void sota_main(int sockfd)
{
	int rcnt;
	char chunk[JSON_CHUNK_SIZE];
	int sec = 0, fd = 0;


	while(1) {
		rcnt = recv_json_file_object(sockfd, "register_client.json");
		sleep(1);

		if(rcnt == 0) {
			break; /* client has closed the connection */
		}

		fd = open("register_client.json", O_RDONLY);
		if(fd < 0) {
			printf("unable to open file %s\n",
			       "register_client.json");
			continue;
		}


		rcnt = read(fd, chunk, JSON_CHUNK_SIZE);
		if(rcnt > JSON_CHUNK_SIZE)
			rcnt = (JSON_CHUNK_SIZE - 1);
		chunk[rcnt+1] = '\0';

		printf("%s\n\n%d\n", chunk, sec++);
	}
	close(fd);

}
