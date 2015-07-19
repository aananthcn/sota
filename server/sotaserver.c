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
#include <errno.h>
#include <string.h>

#include "sotajson.h"
#include "sotadb.h"
#include "sotaserver.h"
#include "sotacommon.h"


/* Global variables */
struct client Client;


/*
 * returns 1 if success, 0 if not, -1 on for errors
 */
int process_hello_msg(json_t *root, char *file)
{
	int id, ret, result = 0;
	char msgdata[JSON_NAME_SIZE];
	json_t *ojson;

	if(root == NULL)
		return -1;

	/* extract login details */
	sj_get_string(root, "vin", Client.vin);
	sj_get_string(root, "message", msgdata);
	sj_get_int(root, "id", &Client.id);

	/* check with database if this vin exist */
	ret = db_get_columnint_fromkeystr(SOTATBL_VEHICLE, "id", &id,
					    "vin", Client.vin);
	if(ret < 0) {
		printf("error in database search\n");
		return -1;
	}

	ret = sj_create_header(&ojson, "login response", 1024);
	if(ret < 0) {
		printf("header creation failed\n");
		return -1;
	}

	/* populate data */
	if(id == Client.id) {
		/* send login success message */
		sj_add_string(&ojson, "message", "login success");
		result = 1;
	}
	else {
		/* send failure response -- update HLD */
		sj_add_string(&ojson, "message", "login failure");
		result = 0;
	}

	/* save the response in file to send later */
	if(0 > sj_store_file(ojson, file)) {
		printf("Could not store regn. result\n");
		return -1;
	}

	return result;
}


int handle_client_registration(json_t* ijson, char *ofile)
{
	json_t *ojson;
	struct client_tbl_row row;
	int ret = 0;
	int scnt;
	int id, vinr;

	ret += sj_get_string(ijson, "vin", row.vin);
	ret += sj_get_string(ijson, "serial_no", row.serial_no);
	ret += sj_get_string(ijson, "name", row.name);
	ret += sj_get_string(ijson, "phone", row.phone);
	ret += sj_get_string(ijson, "email", row.email);
	ret += sj_get_string(ijson, "make", row.make);
	ret += sj_get_string(ijson, "model", row.model);
	ret += sj_get_string(ijson, "device", row.device);
	ret += sj_get_string(ijson, "variant", row.variant);
	ret += sj_get_int(ijson, "year", &row.year);

	if(ret < 0) {
		printf("failed to extract minimum info from json file\n");
		return -1;
	}

	ret = sj_create_header(&ojson, "registration result", 1024);
	if(ret < 0) {
		printf("header creation failed\n");
		return -1;
	}

	/* check if vin number is in the table */
	vinr = db_check_col_str(SOTATBL_VEHICLE, "vin", &row.vin[0]);
	if(vinr < 0) {
		printf("database search failed\n");
		return -1;
	}
	else if(vinr > 0) {
		/* found: get the registration ID of the vin */
		ret = db_get_columnint_fromkeystr(SOTATBL_VEHICLE, "id",
						  &id, "vin", row.vin);
		if(ret < 0) {
			printf("database search for id failed\n");
			return -1;
		}

		/* populate data */
		sj_add_string(&ojson, "message", "already registered");
		sj_add_int(&ojson, "id", id);
		sj_add_string(&ojson, "vin", row.vin);
	}
	else {
		/* not found: insert current vin */
		if(0 > db_insert_row(SOTATBL_VEHICLE, &row))
			return -1;
		printf("Added data to table in MYSQL successfully\n");

		/* get id of the newly inserted vin */
		ret = db_get_columnint_fromkeystr(SOTATBL_VEHICLE, "id",
						  &id, "vin", row.vin);
		if(ret < 0) {
			printf("database search for id failed\n");
			return -1;
		}

		/* populate data */
		sj_add_string(&ojson, "message", "registration success");
		sj_add_int(&ojson, "id", id);
		sj_add_string(&ojson, "vin", row.vin);
	}

	/* save the response in file to send later */
	if(0 > sj_store_file(ojson, ofile)) {
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
	rcnt = sj_recv_file_object(sockfd, ifile);
	if(rcnt <= 0) {
		printf("Client closed connection\n");
		return -1;
	}

	/* load the json file and extract the message type */
	if(0 > sj_load_file(ifile, &root))
		return -1;
	if(0 > sj_get_string(root, "msg_name", msgname))
		return -1;

	/* process client's message */
	if(0 == strcmp(msgname, "client registration")) {
		ret = handle_client_registration(root, rfile);
		if(ret < 0) {
			return -1;
		}

		/* send client registration successful */
		scnt = sj_send_file_object(sockfd, rfile);
		if(scnt <= 0) {
			return -1;
		}
		return 0;
	}
	else if (0 == strcmp(msgname, "client login")) {
		ret = process_hello_msg(root, hfile);
		if(ret < 0) {
			return -1;
		}

		/* send login response */
		scnt = sj_send_file_object(sockfd, hfile);
		if(scnt <= 0) {
			return -1;
		}
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
		printf("I am in query state\n");
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
