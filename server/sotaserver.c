/* 
 * Author: Aananth C N
 * email: c.n.aananth@gmail.com
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
#include <unistd.h>

#include "sotajson.h"
#include "sotadb.h"
#include "sotaserver.h"
#include "sotacommon.h"


/* Global variables */
struct client		Client;
struct download_info	DownloadInfo;
char			SessionPath[JSON_NAME_SIZE];


static SERVER_STATES_T NextState, CurrState;


/*
 * returns 1 if success, 0 if not, -1 on for errors
 */
int populate_part_info(char *ofile, char *bfile, int part)
{
	json_t *jsonf;
	int ret, len;
	FILE *fp;
	char *sp;
	char sha256sum[JSON_NAME_SIZE];
	char cmd_buf[JSON_NAME_SIZE];

	/* compute sha256sum for the part */
	sprintf(cmd_buf, "sha256sum %s > %s/sha256sum.%d", bfile,
		SessionPath, part);
	system(cmd_buf);

	/* capture the sha256 value to DownloadInfo structure */
	sprintf(cmd_buf, "%s/sha256sum.%d", SessionPath, part);
	printf("%s\n", cmd_buf);
	cut_sha256sum_fromfile(cmd_buf, sha256sum, JSON_NAME_SIZE);

	/* populate message header */
	ret = sj_create_header(&jsonf, "download part x", 1024);
	if(ret < 0) {
		printf("header creation failed\n");
		return -1;
	}
	sj_add_string(&jsonf, "sha256sum", sha256sum);
	sj_add_int(&jsonf, "part", part);
	sj_add_string(&jsonf, "message", "download info for file part");

	/* save the response in file to send */
	if(0 > sj_store_file(jsonf, ofile)) {
		printf("Could not store regn. result\n");
		return -1;
	}

	return 1;
}

/*
 * returns 1 if success, 0 if not, -1 on for errors
 */
int prepare_parts_n_list(strname_t **list)
{
	int parts, i=0;
	int allocsize;
	char *pn, *pe;
	char cmd_buf[JSON_NAME_SIZE];
	char cwd[JSON_NAME_SIZE];

	FILE *fp;
	struct stat st = {0};

	/* change dir before splitting files */
	if(getcwd(cwd, sizeof(cwd)) == NULL)
		return -1;
	if(0 > chdir(SessionPath))
		return -1;

	/* split files into smaller parts */
	printf("   splitting files...\n\t");
	sprintf(cmd_buf, "split -b %d %s sw_part_", SOTA_FILE_PART_SIZE,
		DownloadInfo.compdiffpath);
	system(cmd_buf);

	/* create list */
	sprintf(cmd_buf, "ls sw_part_* > part_list.txt");
	system(cmd_buf);

	/* allocate memory for the list */
	parts = DownloadInfo.fileparts;
	allocsize = sizeof(strname_t) * (parts + 1); /* +1 for last part */
	*list = (strname_t *) malloc(allocsize);
	if(*list == NULL) {
		printf("memory allocation failed\n");
		return -1;
	}

	/* populate list */
	fp = fopen("part_list.txt", "r");
	if(fp == NULL) {
		printf("%s(): error opening part list\n", __FUNCTION__);
		goto exit_error;
	}
	do {
		/* read a line */
		pe = fgets((*list)[i], JSON_NAME_SIZE, fp);

		/* eliminate new line character */
		pn = memchr((*list)[i], '\n', sizeof(strname_t));
		if(pn)
			*pn = '\0';

		/* prepare for next read from file */
		i++;
		if(i > parts) {
			break;
		}
	} while (pe != NULL);
	fclose(fp);

	if(i != (parts+1)) {
		printf("%s(): parts and lines doesn't match\n", __FUNCTION__);
		goto exit_error;
	}

	/* change directory back to original */
	chdir(cwd);
	return 0;

exit_error:
	/* change directory back to original */
	chdir(cwd);
	return -1;
}



/*
 * returns next state or -1 for errors
 */
int handle_download_state(int sockfd)
{
	strname_t *parts_list = NULL;
	int rcnt, scnt, ret;
	int id, x, size;
	char msgdata[JSON_NAME_SIZE];
	char vin[JSON_NAME_SIZE];
	char binfile[JSON_NAME_SIZE];
	char ifile[JSON_NAME_SIZE];
	char ofile[JSON_NAME_SIZE];
	json_t *jsonf;

	/* init file paths */
	sprintf(ifile, "%s/%s", SessionPath, "request_part_x.json");
	sprintf(ofile, "%s/%s", SessionPath, "download_info_x.json");

	/* split files and store names in list */
	if(0 > prepare_parts_n_list(&parts_list)) {
		printf("%s(), error in preparing file parts\n",
		       __FUNCTION__);
		return -1;
	}

	do {
		/* receive a message */
		rcnt = sj_recv_file_object(sockfd, ifile);
		if(rcnt <= 0) {
			printf("Client closed connection\n");
			return -1;
		}

		/* load the json file and extract the message contents */
		if(0 > sj_load_file(ifile, &jsonf))
			return -1;
		if(0 > sj_get_string(jsonf, "msg_name", msgdata))
			return -1;
		sj_get_string(jsonf, "vin", vin);
		sj_get_int(jsonf, "id", &id);


		/* process client's message */
		if(0 != strcmp(msgdata, "request part x")) {
			if(0 == strcmp(msgdata, "bye server"))
				if(id == Client.id)
					break;
			printf("%s(): message not valid!\n",
			       __FUNCTION__);
			return -1;
		}

		/* verify id, vin and extract part number */
		if((0 != strcmp(Client.vin, vin)) || (id != Client.id)) {
			printf("%s(): message validation failed\n",
			       __FUNCTION__);
			return -1;
		}
		sj_get_int(jsonf, "part", &x);
		if(x > DownloadInfo.fileparts+1) {
			printf("%s(), cannot handle this request\n",
			       __FUNCTION__);
			break;
		}

		/* send the details of the bin part in json file */
		sprintf(binfile, "%s/%s", SessionPath, parts_list[x]);
		printf("%s\n", binfile);
		if(0 > populate_part_info(ofile, binfile, x))
			return -1;

		/* send download_inf_x.json */
		scnt = sj_send_file_object(sockfd, ofile);
		if(scnt <= 0) {
			printf("error while sending %s\n", ofile);
			return -1;
		}

		/* send the binary data */
		size = get_filesize(binfile);
		scnt = sb_send_file_object(sockfd, binfile, size);
		if(scnt <= 0) {
			printf("error while sending %s\n", binfile);
			return -1;
		}
	} while (1);
	/* receive ack json file */

	return SS_FINAL_STATE;
}

int update_download_info(char *pathc, char *pathn)
{
	int fe_c, fe_n, len;
	int size_c, size_n;
	FILE *fp;
	char cmd_buf[JSON_NAME_SIZE], *sp;

	printf("Updating download Info:\n");
	/* access files */
	fe_c = access(pathc, F_OK);
	fe_n = access(pathn, F_OK);

	if((fe_c != 0) || (fe_n != 0)) {
		printf("Unable to access files to compute diff\n");
		printf("\tnew: \"%s\"\n\told: \"%s\"\n", pathn, pathc);
		return -1;
	}

	/* check if the compression is bzip2 */
	printf("  accessing files...\n");
	len = strlen(pathc);
	if(0 != strcmp(pathc+len-3, "bz2")) {
		printf("curr release package compression is not bz2\n");
		return -1;
	}
	len = strlen(pathn);
	if(0 != strcmp(pathn+len-3, "bz2")) {
		printf("new release package compression is not bz2\n");
		return -1;
	}

	/* copy and uncompress original files */
	printf("  copying files...\n\t");
	sprintf(cmd_buf, "cp -f %s %s/cur.tar.bz2", pathn, SessionPath);
	system(cmd_buf);
	printf("\t");
	sprintf(cmd_buf, "cp -f %s %s/new.tar.bz2", pathc, SessionPath);
	system(cmd_buf);
	printf("  uncompressing file1...\n\t");
	sprintf(cmd_buf, "bzip2 -d %s/cur.tar.bz2", SessionPath);
	system(cmd_buf);
	printf("  uncompressing file2...\n\t");
	sprintf(cmd_buf, "bzip2 -d %s/new.tar.bz2", SessionPath);
	system(cmd_buf);

	/* find the original new file size */
	printf("  computing uncompressed file size...\n");
	sprintf(cmd_buf, "%s/new.tar", SessionPath);
	DownloadInfo.origsize = get_filesize(cmd_buf);
	if(DownloadInfo.origsize < 0)
		return -1;

	/* find the delta */
	printf("  preparing diff file...\n\t");
	sprintf(cmd_buf, "jdiff -b %s/cur.tar %s/new.tar %s/diff.tar",
		SessionPath, SessionPath, SessionPath);
	system(cmd_buf);

	/* compress the diff file */
	DownloadInfo.compression_type = SOTA_BZIP2;
	printf("  compressing diff file...\n\t");
	sprintf(cmd_buf, "bzip2 -z %s/diff.tar", SessionPath);
	system(cmd_buf);

	/* find the diff file size */
	printf("  computing diff file size...\n");
	sprintf(DownloadInfo.compdiffpath, "%s/diff.tar.bz2", SessionPath);
	DownloadInfo.compdiffsize = get_filesize(DownloadInfo.compdiffpath);
	if(DownloadInfo.compdiffsize < 0)
		return -1;

	/* find number of chunks and last chunk size */
	printf("  computing the number of parts to be sent...\n");
	DownloadInfo.fileparts = DownloadInfo.compdiffsize /
		SOTA_FILE_PART_SIZE;
	DownloadInfo.lastpartsize = DownloadInfo.compdiffsize %
		SOTA_FILE_PART_SIZE;

	/* find the sha256 value  */
	printf("  computing sha256sum...\n\t");
	sprintf(cmd_buf, "sha256sum %s/diff.tar.bz2 > %s/diff.sum",
		SessionPath, SessionPath);
	system(cmd_buf);

	/* capture the sha256 value to DownloadInfo structure */
	sprintf(cmd_buf, "%s/diff.sum", SessionPath);
	if(0 > cut_sha256sum_fromfile(cmd_buf, DownloadInfo.sha256sum,
				      JSON_NAME_SIZE))
		return -1;

	printf("%s() -- clean this code\n", __FUNCTION__);
#if 0
	fp = fopen(cmd_buf, "r");
	if(fp == 0) {
		printf("Can't open %s\n", cmd_buf);
		return -1;
	}
	fgets(DownloadInfo.sha256sum, JSON_NAME_SIZE, fp);
	fclose(fp);

	/* retain sha256 sum string alone */
	sp = memchr(DownloadInfo.sha256sum, ' ', JSON_NAME_SIZE);
	len = sp - DownloadInfo.sha256sum;
	DownloadInfo.sha256sum[len] = '\0';
#endif
	printf("... done!\n");

	return 1;
}

/*************************************************************************
 * Function: This function is called for sending positive response for the
 * update request query.
 *
 * arg1: double point to the json object to be sent.
 *
 * return: 1 or 0 if success, -1 on for errors
 */
int populate_update_info(json_t **jp)
{
	int ret;
	char new_version[JSON_NAME_SIZE];
	char pathn[JSON_NAME_SIZE];
	char pathc[JSON_NAME_SIZE];

	/* find new version string */
	ret = db_get_columnstr_fromkeystr(SOTATBL_VEHICLE, "new_sw_version",
					  new_version, "vin", Client.vin);
	if(ret < 0) {
		printf("database search for sw_version failed\n");
		return -1;
	}
	else if(new_version[0] == '\0') {
		printf("incorrect new software version, can't proceed\n");
		return 0;
	}
	else {
		strcpy(DownloadInfo.new_version, new_version);
	}

	/* find path for new version string */
	ret = db_get_columnstr_fromkeystr(SOTATBL_SWRELES, "path", pathn,
				  "sw_version", DownloadInfo.new_version);
	if(ret < 0) {
		printf("database search for sw_version failed\n");
		return -1;
	}

	/* find path for current / old version string */
	ret = db_get_columnstr_fromkeystr(SOTATBL_SWRELES, "path", pathc,
				  "sw_version", Client.sw_version);
	if(ret < 0) {
		printf("database search for curr sw_version failed\n");
		return -1;
	}
	else if(Client.sw_version[0] == '\0') {
		printf("incorrect new software version, can't proceed\n");
		return 0;
	}

	/* check for valid version info and proceed for +ve response */
	if(0 == strcmp(Client.sw_version, DownloadInfo.new_version)) {
		printf("old and new software are same!\n");
		return 0;
	}

	if(0 > update_download_info(pathc, pathn)) {
		printf("update download info failed!\n");
		return -1;
	}

	/* populate update info details to json file */
	sj_add_string(jp, "message", "updates available for you");
	sj_add_string(jp, "new_version", DownloadInfo.new_version);
	sj_add_int(jp, "original_size", DownloadInfo.origsize);
	sj_add_int(jp, "compress_type", DownloadInfo.compression_type);
	sj_add_int(jp, "compressed_diff_size", DownloadInfo.compdiffsize);
	sj_add_int(jp, "file_parts", DownloadInfo.fileparts);
	sj_add_int(jp, "lastpart_size", DownloadInfo.lastpartsize);
	sj_add_string(jp, "sha256sum", DownloadInfo.sha256sum);

	return 1;
}

/*
 * returns 1 if success, 0 if not, -1 on for errors
 */
int identify_updates(json_t *jsonf, char *ofile)
{
	int ret, id, result = 0;
	char msgdata[JSON_NAME_SIZE];
	char vin[JSON_NAME_SIZE];
	int update;
	char thismessage[] = "send available updates";
	json_t *ojson;

	if(jsonf == NULL) {
		printf("%s(): invalid json file passed\n", __FUNCTION__);
		return -1;
	}

	/* extract login details */
	sj_get_string(jsonf, "vin", vin);
	sj_get_string(jsonf, "message", msgdata);
	sj_get_int(jsonf, "id", &id);
	sj_get_string(jsonf, "sw_version", Client.sw_version);

	/* validate the message */
	if((id != Client.id) || (0 != strcmp(vin, Client.vin))) {
		printf("%s(): incorrect client!!\n", __FUNCTION__);
		return -1;
	}

	/* check with database if this vin exist */
	ret = db_get_columnint_fromkeystr(SOTATBL_VEHICLE, "update_available",
					  &update, "vin", Client.vin);
	if(ret < 0) {
		printf("error in database search\n");
		return -1;
	}

	/* populate message header */
	ret = sj_create_header(&jsonf, "software updates info", 1024);
	if(ret < 0) {
		printf("header creation failed\n");
		return -1;
	}

	/* check if update query succeeded */
	if((0 == strcmp(msgdata, thismessage) && (update == 1))) {
		/* if yes - populate query result and info */
		ret =  populate_update_info(&jsonf);
		if(ret > 0) {
			/* save file and return positive */
			if(0 > sj_store_file(jsonf, ofile)) {
				printf("Could not store regn. result\n");
				return -1;
			}
			return 1;
		}
		else
			printf("populate update info failed\n");

	}
	else {
		/* do nothing */
	}

	/* if no - populate negative message */
	sj_add_string(&jsonf, "message", "you are up to date");

	/* save the file */
	if(0 > sj_store_file(jsonf, ofile)) {
		printf("Could not store regn. result\n");
		return -1;
	}

	return 0;
}

/*
 * returns next state or -1 for errors
 */
int handle_query_state(int sockfd)
{
	int rcnt, scnt, ret;
	char msgname[JSON_NAME_SIZE];
	char ifile[JSON_NAME_SIZE];
	char ofile[JSON_NAME_SIZE];
	json_t *jsonf;

	/* init file paths */
	sprintf(ifile, "%s/%s", SessionPath, "request_updates_info.json");
	sprintf(ofile, "%s/%s", SessionPath, "updates_info.json");

	/* receive a message */
	rcnt = sj_recv_file_object(sockfd, ifile);
	if(rcnt <= 0) {
		printf("Client closed connection\n");
		return -1;
	}

	/* load the json file and extract the message type */
	if(0 > sj_load_file(ifile, &jsonf))
		return -1;
	if(0 > sj_get_string(jsonf, "msg_name", msgname))
		return -1;

	/* process client's message */
	if(0 == strcmp(msgname, "software update query")) {
		ret = identify_updates(jsonf, ofile);
		if(ret < 0) {
			printf("identify updates failed!\n");
			return -1;
		}

		/* send updates query result */
		scnt = sj_send_file_object(sockfd, ofile);
		if(scnt <= 0) {
			printf("error while sending %s\n", ofile);
			return -1;
		}

		if(ret)
			return SS_DWNLD_STATE;
		else
			return SS_FINAL_STATE;
	}
	else
		printf("%s(): message name not valid!\n", __FUNCTION__);

	return SS_QUERY_STATE;
}


/*
 * returns 1 if success, 0 if not, -1 on for errors
 */
int process_hello_msg(json_t *jsonf, char *file)
{
	int id, ret, result = 0;
	char msgdata[JSON_NAME_SIZE];
	json_t *ojson;

	if(jsonf == NULL)
		return -1;

	/* extract login details */
	sj_get_string(jsonf, "vin", Client.vin);
	sj_get_string(jsonf, "message", msgdata);
	sj_get_int(jsonf, "id", &Client.id);

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
	if((id == Client.id) && (0 == strcmp(msgdata, "login request"))) {
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
	ret += sj_get_string(ijson, "sw_version", row.cur_sw_version);

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
		sj_add_string(&ojson, "sw_version", row.cur_sw_version);
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
		sj_add_string(&ojson, "sw_version", row.cur_sw_version);
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
	char ifile[JSON_NAME_SIZE];
	char rfile[JSON_NAME_SIZE];
	char hfile[JSON_NAME_SIZE];
	json_t *jsonf;

	/* init file paths */
	sprintf(ifile, "%s/%s", SessionPath, "client.json");
	sprintf(rfile, "%s/%s", SessionPath, "registration_result.json");
	sprintf(hfile, "%s/%s", SessionPath, "hello_client.json");

	/* receive a message */
	rcnt = sj_recv_file_object(sockfd, ifile);
	if(rcnt <= 0) {
		printf("Client closed connection\n");
		return -1;
	}

	/* load the json file and extract the message type */
	if(0 > sj_load_file(ifile, &jsonf))
		return -1;
	if(0 > sj_get_string(jsonf, "msg_name", msgname))
		return -1;

	/* process client's message */
	if(0 == strcmp(msgname, "client registration")) {
		ret = handle_client_registration(jsonf, rfile);
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
		ret = process_hello_msg(jsonf, hfile);
		if(ret < 0) {
			return -1;
		}

		/* send login response */
		scnt = sj_send_file_object(sockfd, hfile);
		if(scnt <= 0) {
			return -1;
		}

		/* login done, expect query from client */
		return SS_QUERY_STATE;
	}

	return SS_INIT_STATE;
}


int process_server_statemachine(int sockfd)
{
	int ret;

	/* curr and next states can be different between one call only */
	CurrState = NextState;

	switch(CurrState) {
	case SS_INIT_STATE:
		ret = handle_init_state(sockfd);
		if(ret < 0)
			goto error_exit;
		else
			NextState = ret;
		break;

	case SS_QUERY_STATE:
		ret = handle_query_state(sockfd);
		if(ret < 0)
			goto error_exit;
		else
			NextState = ret;
		break;

	case SS_DWNLD_STATE:
		ret = handle_download_state(sockfd);
		if(ret < 0)
			goto error_exit;
		else
			NextState = ret;
		break;

	case SS_FINAL_STATE:
		printf("Implement final state handling, Aananth\n");
		break;

	default:
		printf("%s(): Reached invalid state\n", __FUNCTION__);
		goto error_exit;
		break;
	}

	return 0;

error_exit:
	printf("Error in %d state, next state is %d\n", CurrState, NextState);
	CurrState = NextState = 0;
	return -1;
}


void sota_main(int sockfd)
{
	int ret;
	int fe;
	char cmd_buf[JSON_NAME_SIZE];

	printf("\n\nStart of session %lu\n", Sessions);

	/* store files in unique paths to support simultaneous sessions */
	sprintf(SessionPath, "/tmp/sota-%lu", Sessions);
	create_dir(SessionPath);

	do {
		ret = process_server_statemachine(sockfd);

		/* condition check for cd to vin specific local folder */
		if((CurrState == SS_INIT_STATE) &&
		   (NextState == SS_QUERY_STATE)) {
			/* create vin dir if not exist */
			sprintf(cmd_buf, "/tmp/sota-%s", Client.vin);
			fe = access(cmd_buf, F_OK);
			if(fe != 0) {
				create_dir(cmd_buf);
			}

			/* move all temp files for this vin to vin dir */
			sprintf(cmd_buf, "mv %s/* /tmp/sota-%s", SessionPath,
				Client.vin);
			system(cmd_buf);
			rmdir(SessionPath);
			sprintf(SessionPath, "/tmp/sota-%s", Client.vin);
		}
		sleep(1);

	} while(ret >= 0);
}
