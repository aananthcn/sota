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
struct download_info DownloadInfo;
char SessionPath[JSON_NAME_SIZE];



/* 
 * Function: Computes sha256sum on bfile and compare the output value with
 * sha256sum stored in rfile.
 *
 * arg1: json file
 * arg2: binary file
 *
 * returns 1 if success, 0 if not, -1 on for errors
 */
int compare_checksum_x(char *rfile, char *bfile)
{
	int len;
	FILE *fp;
	char *sp;
	json_t *jsonf;
	char cmd_buf[JSON_NAME_SIZE];
	char msgname[JSON_NAME_SIZE];
	char sh1[JSON_NAME_SIZE];
	char sh2[JSON_NAME_SIZE];

	/* load the json file and extract the message type & sha */
	if(0 > sj_load_file(rfile, &jsonf))
		return -1;
	if(0 > sj_get_string(jsonf, "msg_name", msgname))
		return -1;
	if(0 > sj_get_string(jsonf, "sha256sum", sh1))
		return -1;

	/* compute sha256 sum value for the binary file*/
	sprintf(cmd_buf, "sha256sum %s > %s.sum", bfile, bfile);
	system(cmd_buf);

	/* capture the sha256 value to DownloadInfo structure */
	sprintf(cmd_buf, "%s.sum", bfile);
	fp = fopen(cmd_buf, "r");
	if(fp == 0) {
		printf("Can't open %s\n", cmd_buf);
		return -1;
	}
	fgets(sh2, JSON_NAME_SIZE, fp);
	fclose(fp);

	/* retain sha256 sum string alone */
	sp = memchr(sh2, ' ', JSON_NAME_SIZE);
	len = sp - sh2;
	sh2[len] = '\0';

	if((0 == strcmp(msgname, "download part x") && (0 == strcmp(sh1, sh2))))
		return 1;

	return 0;
}


/*
 * returns 1 if success, 0 if not, -1 on for errors
 */
int download_part_x(int sockfd, int x, char *rfile, char *bfile, int size)
{
	json_t *jsonf;
	int tcnt, ret;
	char msgdata[JSON_NAME_SIZE];
	char sfile[JSON_NAME_SIZE]; /* file to send */

	/* init paths & buffers */
	sprintf(sfile, "%s/request_part_x.json", SessionPath);
	sprintf(msgdata,"send part %d of %d", x, DownloadInfo.fileparts+1);

	/* populate data for getting updates info */
	ret = sj_create_header(&jsonf, "request part x", 1024);
	if(ret < 0) {
		printf("header creation failed\n");
		return -1;
	}
	sj_add_int(&jsonf, "id", this.id);
	sj_add_int(&jsonf, "part", x);
	sj_add_string(&jsonf, "vin", this.vin);
	sj_add_string(&jsonf, "message", msgdata);

	/* save the response in file to send */
	if(0 > sj_store_file(jsonf, sfile)) {
		printf("Could not store regn. result\n");
		return -1;
	}

	/* send request_part_x.json */
	tcnt = sj_send_file_object(sockfd, sfile);
	if(tcnt <= 0) {
		printf("Connection with server closed while Tx\n");
		return -1;
	}

	/* receive download_part_x.json */
	tcnt = sj_recv_file_object(sockfd, rfile);
	if(tcnt <= 0) {
		printf("connection with server closed while rx\n");
		return -1;
	}

	/* receive binary data of diff part N */
	tcnt = sb_recv_file_object(sockfd, bfile, size);
	if(tcnt <= 0) {
		printf("connection with server closed while rx\n");
		return -1;
	}

	return 1;
}



/*
 * returns 1 if success, 0 if not, -1 on for errors
 */
int extract_download_info(char *ifile)
{
	int fe, ret;
	json_t *jsonf;

	fe = access(ifile, F_OK);
	if(fe != 0) {
		printf("%s(): %s\n", __FUNCTION__, strerror(errno));
		return -1;
	}

	ret = sj_load_file(ifile, &jsonf);
	if(ret < 0) {
		printf("%s(), Error loading json file %s\n",
		       __FUNCTION__, ifile);
		return -1;
	}

	sj_get_string(jsonf, "new_version", DownloadInfo.new_version);
	sj_get_string(jsonf, "sha256sum", DownloadInfo.sha256sum);
	sj_get_int(jsonf, "original_size", &DownloadInfo.origsize);
	sj_get_int(jsonf, "compress_type", &DownloadInfo.compression_type);
	sj_get_int(jsonf, "compressed_diff_size", &DownloadInfo.compdiffsize);
	sj_get_int(jsonf, "file_parts", &DownloadInfo.fileparts);
	sj_get_int(jsonf, "lastpart_size", &DownloadInfo.lastpartsize);

	return 1;
}



/*
 * returns 1 if success, 0 if not, -1 on for errors
 */
int handle_download(int sockfd)
{
	int parts, i;
	int ret;
	char ifile[JSON_NAME_SIZE]; /* file with info */
	char bfile[JSON_NAME_SIZE]; /* binary file to recv data */
	char rfile[JSON_NAME_SIZE]; /* json file to recv checksum */
	int size;

	/* init paths */
	sprintf(ifile, "%s/updates_info.json", SessionPath);

	/* exctract download info */
	if(0 > extract_download_info(ifile)) {
		printf("Can't extract download info\n");
		return -1;
	}

	parts = DownloadInfo.fileparts;
	for(i = 0; i < parts+1;) {
		if(i >= parts)
			size = DownloadInfo.lastpartsize;
		else
			size = SOTA_FILE_PART_SIZE;

		sprintf(rfile, "%s/download_part_%d.json", SessionPath, i);
		sprintf(bfile, "%s/sw_part_%d", SessionPath, i);
		ret = download_part_x(sockfd, i, rfile, bfile, size);
		if(ret < 0) {
			printf("%s() - download failed!\n", __FUNCTION__);
			break;
		}
		else if(ret == 0)
			continue;

		ret = compare_checksum_x(rfile, bfile);
		if(ret > 0) {
			printf("part %d of %d received\n", i, parts);
			i++;
		}
		else
			printf("part %d checksum failed, retrying..\n", i);
	}


	return 0;
}



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
	char ofile[JSON_NAME_SIZE];
	char ifile[JSON_NAME_SIZE];

	/* init paths */
	sprintf(ofile, "%s/request_updates_info.json", SessionPath);
	sprintf(ifile, "%s/updates_info.json", SessionPath);

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
	char ofile[JSON_NAME_SIZE];
	char rfile[JSON_NAME_SIZE];
	char ifile[] = "registration_result.json";

	/* init paths */
	sprintf(ofile, "%s/hello_server.json", SessionPath);
	sprintf(rfile, "%s/hello_client.json", SessionPath);
	if(access(ifile, F_OK) != 0) {
		printf("%s(): can't open %s\n", __FUNCTION__, ifile);
		return -1;
	}

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
		ret = handle_download(sockfd);
		if(ret < 0)
			goto error_exit;
		else
			next_state = SC_FINAL_STATE;
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
	strcpy(SessionPath, "/tmp/sota");

	/* create directory for storing temp files */
	if(stat(SessionPath, &st) == -1) {
		mkdir(SessionPath, 0777);
	}

	do {
		state = process_client_statemachine(sockfd);
		sleep(1);

	} while(state >= 0);
}
