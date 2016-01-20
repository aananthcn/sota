/* 
 * Author: Aananth C N
 * email: c.n.aananth@gmail.com
 *
 * License: MPL v2
 * Date: 12 July 2015
 */
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

#include "sotajson.h"
#include "sotaclient.h"
#include "sotacommon.h"
#include "metrics.h"
#include "sotamulti.h"
#include "unixcommon.h"



/* Global variables */
struct client this;
struct download_info DownloadInfo;
char SessionPath[JSON_NAME_SIZE];
char DownloadDir[JSON_NAME_SIZE];
char RegFile[JSON_NAME_SIZE];

static CLIENT_STATES_T NextState, CurrState;



/*
 * returns next state or -1 for errors
 */
int handle_final_state(SSL *conn)
{
	json_t* jsonf;
	int ret, tcnt;
	char ofile[JSON_NAME_SIZE];


	/* init paths */
	sprintf(ofile, "%s/bye_server.json", SessionPath);

	/* populate data for getting updates info */
	ret = sj_create_header(&jsonf, "bye server", 1024);
	if(ret < 0) {
		print("%s(), header creation failed\n", __func__);
		return -1;
	}
	sj_add_int(&jsonf, "id", this.id);
	sj_add_string(&jsonf, "vin", this.vin);
	sj_add_string(&jsonf, "message", "nice talking to you");

	/* save the response in file to send */
	if(0 > sj_store_file(jsonf, ofile)) {
		print("%s(), could not store regn. result\n", __func__);
		return -1;
	}
	json_decref(jsonf);

	/* send request_updates_info.json */
	tcnt = sj_send_file_object(conn, ofile);
	if(tcnt <= 0) {
		print("%s(), connection with server closed while Tx\n",
		       __func__);
		return -1;
	}

	return SC_CTRLD_STATE;
}


/*
 * This function sends internal status messages to server
 */
int send_client_status(SSL *conn, char *msg)
{
	json_t* jsonf;
	int ret, tcnt, rcnt;
	char ofile[JSON_NAME_SIZE];


	/* init paths */
	sprintf(ofile, "%s/client_status.json", SessionPath);

	/* populate data for server verfication */
	ret = sj_create_header(&jsonf, "client status", 1024);
	if(ret < 0) {
		print("%s(), header creation failed\n", __func__);
		return -1;
	}
	sj_add_int(&jsonf, "id", this.id);
	sj_add_string(&jsonf, "vin", this.vin);

	/* pass the message to server */
	sj_add_string(&jsonf, "message", msg);

	/* save the contents in a file to send */
	if(0 > sj_store_file(jsonf, ofile)) {
		print("%s(), could not store regn. result\n", __func__);
		return -1;
	}
	json_decref(jsonf);

	/* send client_status.json */
	tcnt = sj_send_file_object(conn, ofile);
	if(tcnt <= 0) {
		print("%s(), connection with server closed while Tx\n",
		       __func__);
		return -1;
	}

	/* receive same file as ack to sync */
	rcnt = sj_recv_file_object(conn, ofile);
	if(rcnt <= 0) {
		print("%s(), can't receive ack\n", __func__);
		return -1;
	}

	return 0;
}


void get_patch_cmd(char *buf, char *tool, char *base, char *diff, char *new)
{
	int i, len;
	char toolcpy[JSON_NAME_SIZE];

	len = strlen(tool);
	for(i = 0; (i < len) && (i < JSON_NAME_SIZE); i++) {
		if((tool[i] == ' ') || (tool[i] == '\n')) {
			break;
		}
		toolcpy[i] = tool[i];
	}
	toolcpy[i] = '\0';

	if(0 == strcmp(toolcpy, "xdelta")) {
		sprintf(buf, "%s %s %s %s", tool, diff, base, new);
	}
	else {
		sprintf(buf, "%s %s %s %s", tool, base, diff, new);
	}
}

/*
 * returns 1 if success, 0 if not, -1 on for errors
 */
int recreate_original_file(SSL *conn, struct uinfo *ui)
{
	int i;
	char *tool = NULL;
	char *sha256_ui;

	char difffile[JSON_NAME_SIZE];
	char shdiff_f[JSON_NAME_SIZE];

	char basefile[JSON_NAME_SIZE];
	char fullnewfile[JSON_NAME_SIZE];
	char shfull_f[JSON_NAME_SIZE];

	char sha256[JSON_NAME_SIZE];
	char cmdbuf[JSON_NAME_SIZE];

	char cwd[JSON_NAME_SIZE];

	if(DownloadInfo.compression_type != SOTA_BZIP2) {
		print("%s(): This version supports bzip2 only\n", __func__);
		return -1;
	}

	/* init paths and names */
	sprintf(difffile, "%s/int.diff.tar", this.sw_path);
	sprintf(shdiff_f, "%s/int.diff.sum", SessionPath);

	sprintf(shfull_f, "%s/full.sum", SessionPath);
	sprintf(basefile, "%s/%s", this.sw_path, this.sw_name);
	sprintf(fullnewfile, "%s/%s", this.sw_path, this.sw_name);
	fullnewfile[strlen(fullnewfile)-3] = '\0';
	strcat(fullnewfile, "new.tar");

	/* prepare file name for the outfile */
	send_client_status(conn, "Integrating parts...");
	print("   integrating file parts...\n");
	sprintf(cmdbuf, "cat %s/sw_part_* > %s", DownloadDir, difffile);
	system(cmdbuf);
	if(!Debug) {
		sprintf(cmdbuf, "rm %s/sw_part_*", DownloadDir);
		system(cmdbuf);
	}

	/* verify sha256sum for diff file */
	send_client_status(conn, "Computing sha256sum for diff...");
	print("   computing sha256sum for diff...\n");
	sprintf(cmdbuf, "sha256sum %s > %s", difffile, shdiff_f);
	system(cmdbuf);

	if(0 > cut_sha256sum_fromfile(shdiff_f, sha256, JSON_NAME_SIZE))
		return -1;
	if(0 != strcmp(sha256, DownloadInfo.sh256_diff)) {
		send_client_status(conn, "Checksum mismatch for diff file...");
		print("sha256sum for diff file did not match\n");
		print("Received sum: %s\n", DownloadInfo.sh256_diff);
		print("Computed sum: %s\n", sha256);
		return 0;
	}

	/* decompres files */
	capture(UNCOMPRESSION_TIME);
#if BASE_BZIP2
	// based on the 56mins time to decompres and compress we decided not
	// to compress and store. Just store the base version as tar ball
	sprintf(cmdbuf, "bzip2 -d %s", basefile);
	system(cmdbuf);
#endif

	/* unpack int.diff.tar from the new location */
	if(getcwd(cwd, sizeof(cwd)) == NULL)
		return -1;
	if(0 > chdir(this.sw_path))
		return -1;
	send_client_status(conn, "Unpacking int.diff.tar...");
	print("   unpacking int.diff.tar file...\n");
	sprintf(cmdbuf, "tar xvf %s", difffile);
	system(cmdbuf);
	chdir(cwd);

	/* clean up tmp file */
	sprintf(cmdbuf, "rm -f %s", difffile);
	system(cmdbuf);

	/* re-point difffile to this.ecu_name */
	sprintf(difffile, "%s/%s_diff.tar.bz2", this.sw_path, this.ecu_name);
	send_client_status(conn, "Decompressing diff file...");
	print("   decompressing diff file...\n");
	sprintf(cmdbuf, "bzip2 -d %s", difffile);
	system(cmdbuf);
	capture(UNCOMPRESSION_TIME);

	/* correct extensions as bzip2 will change .tar.bz2 to .tar */
	difffile[strlen(difffile)-4] = '\0';
	if(access(difffile, F_OK) != 0) {
		print("%s() couldn't find %s\n", __func__, difffile);
		return -1;
	}
#if BASE_BZIP2
	basefile[strlen(basefile)-4] = '\0';
	if(access(basefile, F_OK) != 0) {
		print("%s() couldn't find %s\n", __func__, basefile);
		return -1;
	}
#endif

	/* find the patch tool for this ecu */
	for(i = 0; i < ECUs; i++) {
		if(0 == strcmp(ECU_Info[i].ecu_name, this.ecu_name)) {
			tool = ECU_Info[i].patch_tool;
			break;
		}
	}
	if(tool == NULL) {
		print("%s(), cant get patch tool\n", __func__);
	       return -1;
	}

	/* apply patch (jptch basefile delta newfile) */
	send_client_status(conn, "Applying patch to get new file...");
	print("   applying patch...\n");
	capture(PATCH_TIME);
	get_patch_cmd(cmdbuf, tool, basefile, difffile, fullnewfile);
	system(cmdbuf);
	capture(PATCH_TIME);
	if(access(fullnewfile, F_OK) != 0) {
		print("%s() could not access %s\n", __func__, fullnewfile);
		return -1;
	}

#if BASE_ZIP2
	/* restore base version for future, if update fails */
	print("   restore base version for future use...\n");
	capture(COMPRESSION_TIME);
	sprintf(cmdbuf,"bzip2 %s", basefile);
	system(cmdbuf);
	capture(COMPRESSION_TIME);
#endif

	/* verify sha256sum for full file */
	send_client_status(conn, "Computing sha256sum for the new version...");
	print("   computing sha256sum for full...\n");
	capture(VERIFY_TIME);
	sprintf(cmdbuf, "sha256sum %s > %s", fullnewfile, shfull_f);
	system(cmdbuf);
	capture(VERIFY_TIME);
	if(0 > cut_sha256sum_fromfile(shfull_f, sha256, JSON_NAME_SIZE))
		return -1;

	/* get the sha256 full image for this ecu */
	for(i = 0; i < ECUs; i++) {
		if(0 == strcmp(ui[i].ecu_name, this.ecu_name)) {
			sha256_ui = ui[i].new_sha256;
			break;
		}
	}

	if(0 != strcmp(sha256, sha256_ui)) {
		send_client_status(conn, "Checksum mismatch for full file...");
		print("sha256sum for the full file did not match\n");
		print("Received sum: %s\n", sha256_ui);
		print("Computed sum: %s\n", sha256);
		return 0;
	}

	/* clean up tmp files */
	sprintf(cmdbuf, "rm %s/%s_*", this.sw_path, this.ecu_name);
	system(cmdbuf);

	send_client_status(conn, "Download Success!!");
	return 1;
}


/************************************************************************* 
 * Function: Computes sha256sum on bfile and compare the output value with
 * sha256sum stored in rfile.
 *
 * arg1: json file
 * arg2: binary file
 *
 * returns 1 if success, 0 if not, -1 on for errors
 */
int compare_checksum_x(char *rfile, char *bfile, int part)
{
	int len;
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
	if(0 > sj_get_string(jsonf, "sha256sum_part", sh1))
		return -1;
	json_decref(jsonf);

	/* compute sha256 sum value for the binary file*/
	sprintf(cmd_buf, "sha256sum %s > %s/sha256sum.%d", bfile,
		SessionPath, part);
	system(cmd_buf);

	/* capture the sha256 value to DownloadInfo structure */
	sprintf(cmd_buf, "%s/sha256sum.%d", SessionPath, part);
	if(0 > cut_sha256sum_fromfile(cmd_buf, sh2, JSON_NAME_SIZE))
		return -1;

	if((0 == strncmp(msgname, "download part ", 14) &&
	    (0 == strcmp(sh1, sh2))))
		return 1;

	return 0;
}



/*
 * returns 0 or 1 if success, -1 on errors 
 */
int extract_download_fileinfo(char *bfile, char *rfile, int *size)
{
	json_t *jsonf;
	char buff[JSON_NAME_SIZE];

	/* load the json file and extract the message type & sha */
	if(0 > sj_load_file(rfile, &jsonf))
		return -1;
	if(0 > sj_get_string(jsonf, "msg_name", buff))
		return -1;
	if(0 != strncmp(buff, "download part ", 14))
		return -1;
	if(0 > sj_get_string(jsonf, "partname", buff))
		return -1;
	if(0 > sj_get_int(jsonf, "partsize", size))
		return -1;
	json_decref(jsonf);

	sprintf(bfile, "%s/%s", DownloadDir, buff);

	return 0;
}



/*
 * returns 1 if success, 0 if not, -1 on for errors
 */
int download_part_x(SSL *conn, int x, char *rfile, char *bfile)
{
	json_t *jsonf;
	int tcnt, ret, totalx;
	int size = -1;
	char msgdata[JSON_NAME_SIZE];
	char sfile[JSON_NAME_SIZE]; /* file to send */

	/* init paths & buffers */
	totalx = DownloadInfo.fileparts + (DownloadInfo.lastpartsize ? 1 : 0);
	sprintf(sfile, "%s/request_part_x.json", SessionPath);
	sprintf(msgdata,"request part %d", x);

	/* populate data for getting updates info */
	ret = sj_create_header(&jsonf, msgdata, 1024);
	if(ret < 0) {
		print("%s(), header creation failed\n", __func__);
		return -1;
	}
	sj_add_int(&jsonf, "id", this.id);
	sj_add_int(&jsonf, "part", x);
	sj_add_string(&jsonf, "vin", this.vin);

	sprintf(sfile, "%s/request_part_x.json", SessionPath);
	sprintf(msgdata,"send part %d of %d", x, totalx-1);
	sj_add_string(&jsonf, "message", msgdata);

	/* save the response in file to send */
	if(0 > sj_store_file(jsonf, sfile)) {
		print("%s(), could not store regn. result\n", __func__);
		return -1;
	}
	json_decref(jsonf);

	/* send request_part_x.json */
	tcnt = sj_send_file_object(conn, sfile);
	if(tcnt <= 0) {
		print("%s(), connection with server closed while Tx\n",
		       __func__);
		return -1;
	}

	/* receive download_part_x.json to verify sha256 later */
	tcnt = sj_recv_file_object(conn, rfile);
	if(tcnt <= 0) {
		print("%s(), connection with server closed while rx\n",
		       __func__);
		return -1;
	}

	/* prepare to receive binary file */
	if(0 > extract_download_fileinfo(bfile, rfile, &size)) {
		print("%s(), couldn't get download details from %s!\n",
		       __func__, rfile);
		return -1;
	}

	/* receive binary data of diff part N */
	tcnt = sb_recv_file_object(conn, bfile, size);
	if(tcnt <= 0) {
		print("%s(), connection with server closed while rx\n",
		       __func__);
		return -1;
	}

	return 1;
}



/*
 * returns 1 if success, 0 if not, -1 on for errors
 */
int extract_download_info(char *ifile, struct uinfo *ui, int ecus)
{
	int fe, ret;
	json_t *jsonf;

	fe = access(ifile, F_OK);
	if(fe != 0) {
		print("%s(): %s\n", __func__, strerror(errno));
		return -1;
	}

	ret = sj_load_file(ifile, &jsonf);
	if(ret < 0) {
		print("%s(), Error loading json file %s\n", __func__, ifile);
		return -1;
	}

	sj_get_string(jsonf, "sha256sum_diff", DownloadInfo.sh256_diff);
	sj_get_int(jsonf, "original_size", &DownloadInfo.origsize);
	sj_get_int(jsonf, "compress_type", &DownloadInfo.compression_type);
	sj_get_int(jsonf, "int_diff_size", &DownloadInfo.intdiffsize);
	sj_get_int(jsonf, "file_parts", &DownloadInfo.fileparts);
	sj_get_int(jsonf, "lastpart_size", &DownloadInfo.lastpartsize);

	extract_ecu_update_info(ui, ecus, jsonf);

	json_decref(jsonf);
	return 1;
}


/* 
 * returns -1 in case of error
 */
int send_download_complete_msg(SSL *conn)
{
	json_t *jsonf;
	int tcnt;
	int ret;
	char ofile[JSON_NAME_SIZE];

	/* init paths */
	sprintf(ofile, "%s/download_complete.json", SessionPath);

	/* populate data for getting updates info */
	ret = sj_create_header(&jsonf, "download complete", 1024);
	if(ret < 0) {
		print("%s(), header creation failed\n", __func__);
		return -1;
	}
	sj_add_int(&jsonf, "id", this.id);
	sj_add_string(&jsonf, "vin", this.vin);
	sj_add_string(&jsonf, "message", "verifying download");

	/* save the response in file to send */
	if(0 > sj_store_file(jsonf, ofile)) {
		print("%s(), could not store regn. result\n", __func__);
		return -1;
	}
	json_decref(jsonf);

	/* send request_updates_info.json */
	tcnt = sj_send_file_object(conn, ofile);
	if(tcnt <= 0) {
		print("%s(), connection with server closed while Tx\n",
		       __func__);
		return -1;
	}

	return 0;
}


void print_update_summary(struct uinfo *ui)
{
	int i, j;

	for(i = 0; i < ECUs; i++) {
		for(j = 0; j < ECUs; j++) {
			if(0 == strcmp(ECU_Info[i].ecu_name, ui[j].ecu_name)) {
				print("   %s:\n", ui[j].ecu_name);
				print("\t Current sw version: %s\n",
				       ECU_Info[i].sw_version);
				print("\t Downloaded sw version: %s\n",
				       ui[j].new_version);
				print("\n");
				break;
			}
		}
	}
}


/*
 * returns next state or -1 for errors
 */
int handle_download_state(SSL *conn)
{
	int parts, i;
	int ret, result;
	char ifile[JSON_NAME_SIZE]; /* file with info */
	char rfile[JSON_NAME_SIZE]; /* json file to recv checksum */
	char bfile[JSON_NAME_SIZE]; /* binary file to receive */
	char cmdbuf[JSON_NAME_SIZE];
	struct uinfo *ui;

	/* init paths */
	sprintf(ifile, "%s/updates_info.json", SessionPath);

	result = 0;
	ui = malloc(ECUs * sizeof(struct uinfo));
	if(ui == NULL) {
		print("%s(), malloc failed\n", __func__);
		return -1;
	}

	/* find how many software parts to be received from server */
	if(0 > extract_download_info(ifile, ui, ECUs)) {
		print("%s(), can't extract download info\n", __func__);
		result = -1;
		goto exit_this;
	}
	parts = DownloadInfo.fileparts;

	/* find out how much of it are already received */
	sprintf(rfile, "%s/parts_list.txt", SessionPath);
	sprintf(cmdbuf, "ls %s/sw_part_* > %s", DownloadDir, rfile);
	system(cmdbuf);
	i = get_filelines(rfile);
	if(i < 0) {
		print("%s(), can't get line count from %s\n", __func__, rfile);
		result = -1;
		goto exit_this;
	}
	else if(i > 0) {
		/* let us re-receive last part as the last download was
		 * interrupted and hence it can not be trusted */
		i--;
	}

	capture(DOWNLOAD_TIME);
	for(;i < parts;) {
		sprintf(rfile, "%s/download_info_%d.json", SessionPath, i);
		ret = download_part_x(conn, i, rfile, bfile);
		if(ret < 0) {
			print("%s() - download failed!\n", __func__);
			result = -1;
			goto exit_this;
		}
		else if(ret == 0)
			continue;

		ret = compare_checksum_x(rfile, bfile, i);
		if(ret > 0) {
			print("part %d of %d received\n", i, parts-1);
			i++;
		}
		else
			print("part %d checksum failed, retrying..\n", i+1);
	}
	capture(DOWNLOAD_TIME);

	if(0 > send_download_complete_msg(conn)) {
		print("%s(), could not send download complete msg\n",
		       __func__);
		result = -1;
	}

	/* let us extract the difference files and patch */
	ret = recreate_original_file(conn, ui);
	if(ret < 0 ) {
		print("%s(), couldn't recreate files\n", __func__);
		result = -1;
	}
	else if(ret == 0) {
		print("%s(), download verification fail!!\n", __func__);
	}
	else {
		print("Successfully downloaded the update\n");
		print_update_summary(ui);

		sprintf(cmdbuf, "%s/update_info.json", this.sw_path);
		store_update_info(ui, cmdbuf);
	}

exit_this:
	free(ui);
	return (result == -1) ? result : SC_FINAL_STATE;
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
		print("%s(): %s\n", __func__, strerror(errno));
		return -1;
	}

	if(0 < sj_load_file(ifile, &jsonf)) {
		sj_get_string(jsonf, "message", msgdata);
		if(0 != strcmp(msgdata, "updates available for you"))
			return 0;

		json_decref(jsonf);
		return 1;
	}
	else {
		print("%s(), Error loading json file %s\n", __func__, ifile);
	}

	return 0;
}


/*
 * returns next state or -1 for errors
 */
int handle_query_state(SSL *conn)
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
		print("%s(), header creation failed\n", __func__);
		return -1;
	}
	sj_add_int(&jsonf, "id", this.id);
	sj_add_string(&jsonf, "vin", this.vin);
	sj_add_string(&jsonf, "message", "send available updates");

	/* save the response in file to send */
	if(0 > sj_store_file(jsonf, ofile)) {
		print("%s(), could not store regn. result\n", __func__);
		return -1;
	}
	json_decref(jsonf);

	/* send request_updates_info.json */
	tcnt = sj_send_file_object(conn, ofile);
	if(tcnt <= 0) {
		print("%s(), connection with server closed while Tx\n",
		       __func__);
		return -1;
	}

	/* receive updates_info.json */
	tcnt = sj_recv_file_object(conn, ifile);
	if(tcnt <= 0) {
		print("%s(), connection with server closed while Rx\n",
		       __func__);
		return -1;
	}

	/* read updates message response */
	if(check_updates_available(ifile)) {
		print("software updates available!!\n");
		return SC_DWNLD_STATE;
	}
	else {
		print("client's software is up-to-date\n");
	}

	return SC_FINAL_STATE;
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
			json_decref(jsonf);
			if(0 == strcmp(msgdata, "login success"))
				return 1;
		}
		else {
			print("%s(), Error loading json file %s\n", __func__,
			       file);
		}
	}
	else
		print("%s(): %s\n", __func__, strerror(errno));

	return 0;
}


int extract_client_info(char *file)
{
	int fe, i;
	json_t *jsonf;

	fe = access(file, F_OK);
	if(fe != 0) {
		print("%s(), can't access %s, this is very important file\n",
		       __func__, file);
		return -1;
	}

	if(0 > sj_load_file(file, &jsonf)) {
		print("%s(), can't load %s to json object\n", __func__, file);
		return -1;
	}

	if(0 > sj_get_string(jsonf, "name", this.name)) {
		print("%s(), can't get name from json\n", __func__);
		return -1;
	}

	if(0 > sj_get_string(jsonf, "vin", this.vin)) {
		print("%s(), can't get vin from json\n", __func__);
		return -1;
	}

	if(0 > sj_get_string(jsonf, "this_ecu", this.ecu_name)) {
		print("%s(), can't get this_ecu from json\n", __func__);
		return -1;
	}
	tolower_str(this.ecu_name); /* names shall be in lower case */

	if(0 > sj_get_string(jsonf, "sw_path", this.sw_path)) {
		print("%s(), can't get sw_path from json\n", __func__);
		return -1;
	}

	if(0 > sj_get_string(jsonf, "sw_name", this.sw_name)) {
		print("%s(), can't get sw_name from json\n", __func__);
		return -1;
	}
	extract_ecus_info(jsonf);
	json_decref(jsonf);

	if(DownloadDir[0] == 0) {
		/* let's keep the downloaded sw parts also in same path */
		strcpy(DownloadDir, this.sw_path);
	}

	return 1;
}


/*
 * returns next state or -1 for errors
 */
int handle_login_state(SSL *conn, char *cifile)
{
	json_t *jsonf;
	int tcnt;
	int ret;
	char ofile[JSON_NAME_SIZE];
	char rfile[JSON_NAME_SIZE];

	/* init paths */
	sprintf(ofile, "%s/hello_server.json", SessionPath);
	sprintf(rfile, "%s/hello_client.json", SessionPath);
	if(access(RegFile, F_OK) != 0) {
		print("%s(): can't open %s\n", __func__, RegFile);
		return -1;
	}

	/* in case login is attempted before check registration */
	if((this.id == 0) || (this.vin == NULL) || (this.name == NULL)) {
		if( 0 > extract_client_info(cifile))
			return SC_REGTN_STATE;
	}

	/* populate id, vin into a hello_server.json file */
	ret = sj_create_header(&jsonf, "client login", 1024);
	if(ret < 0) {
		print("%s(), header creation failed\n", __func__);
		return -1;
	}
	sj_add_int(&jsonf, "id", this.id);
	sj_add_string(&jsonf, "vin", this.vin);
	sj_add_string(&jsonf, "name", this.name);
	sj_add_string(&jsonf, "message", "login request");

	if(0 > add_multi_ecu_info(jsonf))
		return -1;

	/* save the response in file to send */
	if(0 > sj_store_file(jsonf, ofile)) {
		print("%s(), could not store regn. result\n", __func__);
		return -1;
	}

	/* send hello_server.json */
	tcnt = sj_send_file_object(conn, ofile);
	if(tcnt <= 0) {
		print("%s(), connection with server closed while Tx\n",
		       __func__);
		return -1;
	}

	/* receive response */
	tcnt = sj_recv_file_object(conn, rfile);
	if(tcnt <= 0) {
		print("%s(), connection with server closed while Rx\n",
		       __func__);
		return -1;
	}

	/* read message "login success" */
	if(!check_login_success(rfile)) {
		print("Login failure, check the registration files\n");
		return SC_FINAL_STATE;
	}
	else {
		print("Login success!!\n");
		return SC_QUERY_STATE;
	}

	json_decref(jsonf);
	return 0;
}


/*
 * returns 1 if success, 0 if not, -1 on for errors
 */
int check_registration_done(char *file, char *cifile)
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
			print("%s(), Error loading json file %s\n", __func__,
			       file);
		}
	}

	return 0;

xtract_more_exit:
	if(0 > extract_client_info(cifile)) {
		print("%s(), unable to extract client info from %s\n",
		       __func__, file);
		return 0;
	}

	if(0 > sj_get_int(jsonf, "id", &this.id)) {
		print("%s(), can't get id from json\n", __func__);
		return -1;
	}

	json_decref(jsonf);
	return 1;
}


int prepare_registration_msg(char *ifile, char *ofile)
{
	FILE *fi, *fo;
	int i;
	char buf[JSON_NAME_SIZE];

	if((ifile == NULL) || (ofile == NULL))
		return -1;

	if((fi = fopen(ifile, "r")) == NULL) {
		print("%s(), can't open file: %s\n", ifile, __func__);
		return -1;
	}
	else if((fo = fopen(ofile, "w")) == NULL) {
		print("%s(), can't open file: %s\n", ofile, __func__);
		fclose(fi);
		return -1;
	}

	for(i = 0; fgets(buf, JSON_NAME_SIZE, fi) != NULL; i++) {
		if(i == 1) {
			fputs("\t\"msg_name\": \"client registration\",\n", fo);
			fputs("\t\"msg_size\": 2048,\n", fo);
		}
		fputs(buf, fo);
	}

	fclose(fi);
	fclose(fo);

	return 0;
}


/*
 * returns next state or -1 for errors
 */
int handle_registration_state(SSL *conn, char *cifile)
{
	int tcnt, ret, fe;
	json_t *jsonf;
	char ofile[JSON_NAME_SIZE];

	/* check if client info file is present */
	fe = access(cifile, F_OK);
	if(fe != 0) {
		print("%s(), can't open file %s\n", __func__, cifile);
		return -1;
	}

	if(DownloadDir[0] == 0)
		sprintf(RegFile, "registration_result.json");
	else
		sprintf(RegFile, "%s/registration_result.json", DownloadDir);

	/* check if already registered */
	if(check_registration_done(RegFile, cifile))
		return SC_LOGIN_STATE;

	/* if not, prepare for registration */
	sprintf(ofile, "%s/%s", SessionPath, get_filename(cifile));
	if(0 > prepare_registration_msg(cifile, ofile))
		return -1;

	/* send registration message */
	tcnt = sj_send_file_object(conn, ofile);
	if(tcnt <= 0) {
		print("%s(), connection with server closed while Tx\n",
		       __func__);
		return -1;
	}

	/* receive registration response message */
	tcnt = sj_recv_file_object(conn, RegFile);
	if(tcnt <= 0) {
		print("%s(), connection with server closed while Rx\n",
		       __func__);
		return -1;
	}

	return SC_REGTN_STATE;
}



int process_client_statemachine(SSL *conn, char *cfgfile)
{
	int ret;

	/* curr and next states can be different between one call only */
	CurrState = NextState;

	switch(CurrState) {
	case SC_REGTN_STATE:
		ret = handle_registration_state(conn, cfgfile);
		if(ret < 0)
			goto error_exit;
		else
			NextState = ret;
		break;

	case SC_LOGIN_STATE:
		ret = handle_login_state(conn, cfgfile);
		if(ret < 0)
			goto error_exit;
		else
			NextState = ret;
		break;

	case SC_QUERY_STATE:
		ret = handle_query_state(conn);
		if(ret < 0)
			goto error_exit;
		else
			NextState = ret;
		break;

	case SC_DWNLD_STATE:
		ret = handle_download_state(conn);
		if(ret < 0)
			goto error_exit;
		else
			NextState = ret;
		break;

	case SC_FINAL_STATE:
		ret = handle_final_state(conn);
		if(ret < 0)
			goto error_exit;
		else
			NextState = ret;
		break;

	default:
		print("%s(): Reached invalid state\n", __func__);
		goto error_exit;
		break;
	}

	return 0;

error_exit:
	print("%s(), error in %d state, next state is %d\n", __func__,
	       CurrState, NextState);
	CurrState = NextState = 0;
	return -1;
}



void sota_main(SSL *conn, char *cfgfile, char *tmpd, char *stod)
{
	int state, mins;
	char cmdbuf[JSON_NAME_SIZE];

	/* choose path to store temporary files */
	strcpy(SessionPath, tmpd);

	/* create directory for storing temp files */
	create_dir(SessionPath);

	/* initialise download dir with storage dir passed */
	if(stod != NULL)
		strcpy(DownloadDir, stod);
	else
		DownloadDir[0] = 0;

	capture(TOTAL_TIME);
	do {
		state = process_client_statemachine(conn, cfgfile);
		if(NextState == SC_CTRLD_STATE)
			break;
	} while(state >= 0);

	if(!Debug) {
		/* clean up temporary files */
		print("   deleting temp directory...\n");
		sprintf(cmdbuf, "rm -rf %s", SessionPath);
		system(cmdbuf);
	}
	capture(TOTAL_TIME);

	cleanup_multi_ecu_info();
	print_metrics();
}
