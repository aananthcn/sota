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
#include "sotaserver.h"


int process_hello_msg(char *file)
{
}


int handle_client_registration(char *file)
{
	json_t *ijson;
	json_t *ojson;
	struct client_tbl_row row;
	int ret = 0;

	if(0 > load_json_file(file, &ijson)) {
		return -1;
	}

	ret += get_json_string(ijson, "vin", row.vin);
	ret += get_json_string(ijson, "serial_no", row.serial_no);
	ret += get_json_string(ijson, "name", row.name);
	ret += get_json_string(ijson, "phone", row.phone);
	ret += get_json_string(ijson, "email", row.email);
	ret += get_json_string(ijson, "make", row.make);
	ret += get_json_string(ijson, "model", row.model);
	ret += get_json_string(ijson, "device", row.device);
	ret += get_json_string(ijson, "variant", row.variant);
	ret += get_json_int(ijson, "year", &row.year);

	printf("Client name: %s\n", row.name);

	printf("Extracted data from json file successfully\n");
	if(ret >= 0)
		if(0 > sotadb_add_row(SOTATBL_VEHICLE, &row))
			return -1;

	printf("Added data to table in MYSQL successfully\n");

// need to send	registration_result.json file back to client!!

	return 0;
}


int process_server_statemachine(int sockfd)
{
	int rcnt;
	static SERVER_STATES_T next_state, curr_state;
	char msgname[JSON_NAME_SIZE];
	char file[] = "/tmp/temp.json";

	rcnt = recv_json_file_object(sockfd, file, msgname);
	if(rcnt <= 0) {
		printf("Client closed connection\n");
		goto error_exit;
	}

	switch(curr_state) {
	case SOTA_INIT_STATE:
		if(0 == strcmp(msgname, "client registration")) {
			 handle_client_registration(file);
		}
		else if (0 == strcmp(msgname, "client login")) {
			if(0 >= process_hello_msg(file)) {
				next_state = SOTA_QUERY_STATE;
			}
		}
		break;

	case SOTA_QUERY_STATE:
		break;

	case SOTA_DWNLD_STATE:
		break;

	default:
		printf("%s(): Reached invalid state\n", __FUNCTION__);
		goto error_exit;
		break;
	}

	curr_state = next_state;
	return 0;

error_exit:
	curr_state = next_state = 0;
	return -1;
}

void sota_main(int sockfd)
{
	int state;

	do {
		state = process_server_statemachine(sockfd);
		sleep(1);

	} while(state >= 0);
}
