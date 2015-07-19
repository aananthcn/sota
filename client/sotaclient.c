/* 
 * Author: Aananth C N
 * email: c.n.aananth@gmail.com
 *
 * License: GPL v2
 * Date: 12 July 2015
 */
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "sotajson.h"
#include "sotaclient.h"
#include "sotacommon.h"



/* Global variables */
struct client this;


/*
 * returns 1 if success, 0 if not, -1 on for errors
 */
int check_login_success(char *file)
{
	int fe;
	json_t *root;
	char msgdata[JSON_NAME_SIZE];

	fe = access(file, F_OK);
	if(fe == 0) {
		if(0 < sj_load_file(file, &root)) {
			sj_get_string(root, "message", msgdata);
			if(0 == strcmp(msgdata, "login success"))
				return 1;
		}
		else {
			printf("%s(), Error loading json file %s\n",
			       __FUNCTION__, file);
		}
	}
	else
		printf("%s(): %s\n", __FUNCTION__, strerror(errno));

	return 0;
}


int handle_login(int sockfd)
{
	json_t *jsonf;
	int tcnt;
	int ret;
	char ifile[] = "registration_result.json";
	char ofile[] = "/tmp/hello_server.json";
	char rfile[] = "/tmp/hello_client.json";

	/* load registration_result.json file */
	if(0 > sj_load_file(ifile, &jsonf))
		return -1;
	/* populate id, vin into a hello_server.json file */
	if((this.id == 0) || (this.vin == NULL))
		return -1;

	ret = sj_create_header(&jsonf, "client login", 1024);
	if(ret < 0) {
		printf("header creation failed\n");
		return -1;
	}
	sj_add_int(&jsonf, "id", this.id);
	sj_add_string(&jsonf, "vin", this.vin);
	sj_add_string(&jsonf, "message", "login request");

	/* save the response in file to send */
	if(0 > sj_store_file(jsonf, ofile)) {
		printf("Could not store regn. result\n");
		return -1;
	}

	/* send hello_server.json */
	tcnt = sj_send_file_object(sockfd, ofile);
	if(tcnt <= 0) {
		printf("Connection with server closed while Tx\n");
		return -1;
	}

	/* receive response */
	tcnt = sj_recv_file_object(sockfd, rfile);
	if(tcnt <= 0) {
		printf("Connection with server closed while Rx\n");
		return -1;
	}

	/* read message "login success" */
	if(!check_login_success(rfile)) {
		printf("Login failure, check the registration files\n");
		return 0;
	}
	else {
		printf("Login success!!\n");
		return 1;
	}

	/* on server side, store the login status in memory to query */
	printf("Please implement code to login in server, Aananth\n");
	return 0;
}

/*
 * returns 1 if success, 0 if not, -1 on for errors
 */
int check_registration_done(char *file)
{
	int fe;
	json_t *root;
	char msgdata[JSON_NAME_SIZE];

	fe = access(file, F_OK);
	if(fe == 0) {
		if(0 < sj_load_file(file, &root)) {
			sj_get_string(root, "message", msgdata);
			if(0 == strcmp(msgdata, "registration success"))
				goto xtract_more_exit;
			if(0 == strcmp(msgdata, "already registered"))
				goto xtract_more_exit;
		}
		else {
			printf("%s(), Error loading json file %s\n",
			       __FUNCTION__, file);
		}
	}
	else
		printf("%s(): %s\n", __FUNCTION__, strerror(errno));

	return 0;

xtract_more_exit:
	if(0 > sj_get_string(root, "vin", this.vin))
		return 0;
	if(0 > sj_get_int(root, "id", &this.id))
		return 0;

	return 1;
}


/*
 * returns 1 if success, 0 if not, -1 on for errors
 */
int handle_registration(int sockfd)
{
	int tcnt;
	char rcfile[] = "register_client.json";
	char rrfile[] = "registration_result.json";

	/* check if already registered */
	if(check_registration_done(rrfile))
		return 1;

	/* if not send registration message */
	tcnt = sj_send_file_object(sockfd, rcfile);
	if(tcnt <= 0) {
		printf("Connection with server closed while Tx\n");
		return -1;
	}

	/* receive registration response message */
	tcnt = sj_recv_file_object(sockfd, rrfile);
	if(tcnt <= 0) {
		printf("Connection with server closed while Rx\n");
		return -1;
	}

	/* process response */
	if(check_registration_done(rrfile))
		return 1;

	return 0;
}

int process_client_statemachine(int sockfd)
{
	int ret;
	static CLIENT_STATES_T next_state, curr_state;

	switch(curr_state) {
	case SC_REGTN_STATE:
		ret = handle_registration(sockfd);
		if(ret < 0)
			goto error_exit;
		else if(ret > 0)
			next_state = SC_LOGIN_STATE;
		break;

	case SC_LOGIN_STATE:
		ret = handle_login(sockfd);
		if(ret < 0)
			goto error_exit;
		else if(ret > 0)
			next_state = SC_QUERY_STATE;
		break;

	case SC_QUERY_STATE:
		printf("Please implement code to query, Aananth\n");
		break;

	case SC_DWNLD_STATE:
		printf("Please implement code to download, Aananth\n");
		break;

	case SC_FINAL_STATE:
		printf("Please implement code to say bye, Aananth\n");
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
		state = process_client_statemachine(sockfd);
		sleep(1);

	} while(state >= 0);
}
