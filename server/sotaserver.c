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
	return 0;
}


int handle_client_registration(json_t* ijson, char *ofile)
{
	json_t *ojson;
	struct client_tbl_row row;
	int ret = 0;
	int scnt;

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

	if(ret < 0) {
		printf("failed to extract minimum info from json file\n");
		return -1;
	}

	ret = create_sotajson_header(&ojson, "registration result", 1024);
	if(ret < 0) {
		printf("header creation failed\n");
		return -1;
	}

	ret = db_check_col_str(SOTATBL_VEHICLE, "vin", &row.vin[0]);
	if(ret < 0) {
		printf("database search failed\n");
		return -1;
	}
	else if(ret > 0) {
		/* vin found in database */
		add_json_string(&ojson, "message", "already registered");
	}
	else {
		/* vin NOT found in data base */
		if(0 > db_insert_row(SOTATBL_VEHICLE, &row))
			return -1;
		printf("Added data to table in MYSQL successfully\n");
		add_json_string(&ojson, "message", "registration success");
	}

	/* save the response in file to send later */
	if(0 > store_json_file(ojson, ofile)) {
		printf("Could not store regn. result\n");
		return -1;
	}

	return 0;
}


int handle_init_state(int sockfd)
{
	int rcnt, scnt, ret;
	char msgname[JSON_NAME_SIZE];
	char ifile[] = "/tmp/temp.json";
	char rfile[] = "/tmp/registration_result.json";
	char hfile[] = "/tmp/hello_client.json";
	json_t *root;


	/* receive a message */
	rcnt = recv_json_file_object(sockfd, ifile);
	if(rcnt <= 0) {
		printf("Client closed connection\n");
		return -1;
	}

	/* load the json file and extract the message type */
	if(0 > load_json_file(ifile, &root))
		return -1;
	if(0 > get_json_string(root, "msg_name", msgname))
		return -1;

	/* process client's message */
	if(0 == strcmp(msgname, "client registration")) {
		ret = handle_client_registration(root, rfile);
		if(ret < 0) {
			return -1;
		}

		/* send client registration successful */
		scnt = send_json_file_object(sockfd, rfile);
		if(scnt <= 0) {
			return -1;
		}

		return 0;
	}
	else if (0 == strcmp(msgname, "client login")) {
		if(0 > process_hello_msg(hfile)) {
			return -1;
		}

		/* logic successful */
		return 1;
	}

	return 0;
}


int process_server_statemachine(int sockfd)
{
	int ret;
	static SERVER_STATES_T next_state, curr_state;

	switch(curr_state) {
	case SS_INIT_STATE:
		ret = handle_init_state(sockfd);
		if(ret < 0)
			goto error_exit;
		else if(ret > 0)
			next_state = SS_QUERY_STATE;
		break;

	case SS_QUERY_STATE:
		break;

	case SS_DWNLD_STATE:
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
