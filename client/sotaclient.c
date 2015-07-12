/* 
 * Author: Aananth C N
 * email: caananth@visteon.com
 *
 * License: GPL v2
 * Date: 12 July 2015
 */

#include "sotajson.h"

void sota_main(int sockfd)
{
	while(1) {
		send_json_file_object(sockfd, "register_client.json");
		sleep(1);
	}
}
