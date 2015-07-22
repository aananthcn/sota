/* 
 * Author: Aananth C N
 * email: c.n.aananth@gmail.com
 *
 * License: GPL v2
 * Date: 12 July 2015
 */
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

#include "sotajson.h"
#include "sotaclient.h"
#include "sotacommon.h"



/* Global variables */
struct client this;
char SessionPath[JSON_NAME_SIZE];


int check_updates_available(char *ifile)
{
	int fe;
	json_t *jsonf;
	char msgdata[JSON_NAME_SIZE];
	char cur_version[JSON_NAME_SIZE];
	char new_version[JSON_NAME_SIZE];

	fe = access(ifile, F_OK);
	if(fe != 0) {
		printf("%s(): %s\n", __FUNCTION__, strerror(errno));
		return -1;
	}

	if(0 < sj_load_file(ifile, &jsonf)) {
		sj_get_string(jsonf, "message", msgdata);
		if(0 != strcmp(msgdata, "updates available for you"))
			return 0;

		sj_get_string(jsonf, "new_version", new_version);
		if(0 != strcmp(new_version, this.sw_version))
			return 1;
	}
	else {
		printf("%s(), Error loading json file %s\n",
		       __FUNCTION__, ifile);
	}

	return 0;
}


/*
 * returns 1 if success, 0 if not, -1 on for errors
 */
int get_available_updates(int sockfd)
{
	json_t *jsonf;
	int tcnt;
	int ret;
	char ofile[] = "/tmp/request_updates_info.json";
	char ifile[] = "/tmp/updates_info.json";

	/* populate data for getting updates info */
	ret = sj_create_header(&jsonf, "software update query", 1024);
	if(ret < 0) {
		printf("header creation failed\n");
		return -1;
	}
	sj_add_int(&jsonf, "id", this.id);
	sj_add_string(&jsonf, "vin", this.vin);
	sj_add_string(&jsonf, "sw_version", this.sw_version);
	sj_add_string(&jsonf, "message", "send available updates");

	/* save the response in file to send */
	if(0 > sj_store_file(jsonf, ofile)) {
		printf("Could not store regn. result\n");
		return -1;
	}

	/* send request_updates_info.json */
	tcnt = sj_send_file_object(sockfd, ofile);
	if(tcnt <= 0) {
		printf("Connection with server closed while Tx\n");
		return -1;
	}

	/* receive updates_info.json */
	tcnt = sj_recv_file_object(sockfd, ifile);
	if(tcnt <= 0) {
		printf("Connection with server closed while Rx\n");
		return -1;
	}

	/* read updates message response */
	if(check_updates_available(ifile)) {
		printf("software updates available!!\n");
		return 1;
	}
	else {
		printf("client's software is up-to-date\n");
		return 0;
	}

	return 0;
}



/*
 * returns 1 if success, 0 if not, -1 on for errors
 */
int check_login_success(char *file)
{
	int fe;
	json_t *jsonf;
	char msgdata[JSON_NAME_SIZE];

	fe = access(file, F_OK);
	if(fe == 0) {
		if(0 < sj_load_file(file, &jsonf)) {
			sj_get_string(jsonf, "message", msgdata);
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


int extract_client_info(json_t *jsonf)
{
	if(0 > sj_get_string(jsonf, "vin", this.vin)) {
		printf("can't get vin from json\n");
		return -1;
	}
	if(0 > sj_get_int(jsonf, "id", &this.id)) {
		printf("can't get id from json\n");
		return -1;
	}
	if(0 > sj_get_string(jsonf, "sw_version", this.sw_version)) {
		printf("can't get id from json\n");
		return -1;
	}

	return 1;
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

	/* in case login is attempted before check registration */
	if((this.id == 0) || (this.vin == NULL)) {
		if( 0 > extract_client_info(jsonf))
			return 0;
	}

	/* populate id, vin into a hello_server.json file */
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

	return 0;
}

/*
 * returns 1 if success, 0 if not, -1 on for errors
 */
int check_registration_done(char *file)
{
	int fe;
	json_t *jsonf;
	char msgdata[JSON_NAME_SIZE];

	fe = access(file, F_OK);
	if(fe == 0) {
		if(0 < sj_load_file(file, &jsonf)) {
			sj_get_string(jsonf, "message", msgdata);
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
	if(0 > extract_client_info(jsonf)) {
		printf("%s(), unable to extract client info from %s\n",
		       __FUNCTION__, file);
		return 0;
	}

	return 1;
}


/*
 * returns 1 if success, 0 if not, -1 on for errors
 */
int handle_registration(int sockfd)
{
	int tcnt;
	char cifile[] = "client_info.json";
	char rrfile[] = "registration_result.json";

	/* check if already registered */
	if(check_registration_done(cifile))
		return 1;

	/* if not send registration message */
	tcnt = sj_send_file_object(sockfd, cifile);
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

	printf("registration not successful\n");
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
		ret = get_available_updates(sockfd);
		if(ret < 0)
			goto error_exit;
		else if(ret > 0)
			next_state = SC_DWNLD_STATE;
		else
			next_state = SC_FINAL_STATE;
		break;

	case SC_DWNLD_STATE:
		printf("Please implement code to download, Aananth\n");
		break;

	case SC_FINAL_STATE:
		/* update "client_info.json" file */
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
	printf("Error in %d state, next state is %d\n", curr_state, next_state);
	curr_state = next_state = 0;
	return -1;
}



void sota_main(int sockfd)
{
	int state;
	struct stat st = {0};

	/* chose path to store temporary files */
	strcpy(SessionPath, "/tmp/sota/");

	/* create directory for storing temp files */
	if(stat(SessionPath, &st) == -1) {
		mkdir(SessionPath, 0777);
	}

	do {
		state = process_client_statemachine(sockfd);
		sleep(1);

	} while(state >= 0);
}
