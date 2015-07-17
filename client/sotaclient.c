/* 
 * Author: Aananth C N
 * email: c.n.aananth@gmail.com
 *
 * License: GPL v2
 * Date: 12 July 2015
 */
#include <unistd.h>
#include <errno.h>

#include "sotajson.h"
#include "sotaclient.h"


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
		if(0 < load_json_file(file, &root)) {
			get_json_string(root, "message", msgdata);
			if(0 == strcmp(msgdata, "registration success"))
				return 1;
			if(0 == strcmp(msgdata, "already registered"))
				return 1;
		}
		else {
			printf("%s(), Error loading json file %s\n",
			       __FUNCTION__, file);
		}
	}
	else
		printf("%s(): %s\n", __FUNCTION__,  strerror(errno));

	return 0;
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
	tcnt = send_json_file_object(sockfd, rcfile);
	if(tcnt <= 0) {
		printf("Connection with server closed while Tx\n");
		return -1;
	}

	/* receive registration response message */
	tcnt = recv_json_file_object(sockfd, rrfile);
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
		printf("Please implement code to login to server, Aananth\n");
		break;

	case SC_QUERY_STATE:
		break;

	case SC_DWNLD_STATE:
		break;

	case SC_FINAL_STATE:
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
