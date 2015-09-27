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
#include <time.h>

#include "sotajson.h"
#include "sotadb.h"
#include "sotaserver.h"
#include "sotacommon.h"
#include "sotamulti.h"


char ECU_Table[JSON_NAME_SIZE];



/*************************************************************************
 * Function: add_multi_downloadinfo
 *
 * This function appends multi ecu array to the json file object passed as
 * argument.
 */
int add_multi_ecu_downloadinfo(json_t *jsonf, struct ecu_update_str *ui,
			       int rows)
{
	int i, ret;
	json_t *jarray;
	char sha256[JSON_NAME_SIZE];
	char cmd_buf[JSON_NAME_SIZE];

	jarray = json_array();
	if((jsonf == NULL) || (jarray == NULL)) {
		printf("%s(), invalid argument passed\n", __FUNCTION__);
		return -1;
	}
	json_object_set(jsonf, "ecus", jarray);

	for(i = 0; i < rows; i++) {
		json_t *jrow;

		jrow = json_object();

		/* add ecu name */
		ret = sj_add_string(&jrow, "ecu_name", ui[i].ecu_name);
		if(0 > ret) {
			printf("%s(), can't add \"ecu_name\"!\n", __FUNCTION__);
			return -1;
		}

		/* add update_available info */
		ret = sj_add_int(&jrow, "update_available",
				    ui[i].update_available);
		if(0 > ret) {
			printf("%s(), can't add \"update_available\"!\n",
			       __FUNCTION__);
			return -1;
		}

		/* add new version */
		ret = sj_add_string(&jrow, "new_version", ui[i].new_version);
		if(0 > ret) {
			printf("%s(), can't add \"new_version\"!\n",
			       __FUNCTION__);
			return -1;
		}

		/* add sha256sum for full image */
		ret = sj_add_string(&jrow, "sha256sum_full", ui[i].new_sha256);
		if(0 > ret) {
			printf("%s(), can't add \"new_version\"!\n",
			       __FUNCTION__);
			return -1;
		}
		json_array_append(jarray, jrow);

		json_decref(jrow);
	}

	json_decref(jarray);
	return 0;
}



/***********************************************************************
 * Function: store_diff_info
 *
 * Called by create_diff_image for storage and retrieval of diff data.
 */
int store_diff_info(char *dir, int origsize, struct ecu_update_str *updts)
{
	json_t *jsonf;
	char file[JSON_NAME_SIZE];

	if((dir == NULL) || (updts == NULL))
		return -1;

	sprintf(file, "%s/%s.json", dir, updts->ecu_name);

	sj_create_header(&jsonf, "diff info", 1024);
	sj_add_int(&jsonf, "original_size", origsize);
	sj_add_string(&jsonf, "ecu_name", updts->ecu_name);
	sj_add_string(&jsonf, "new_sha256", updts->new_sha256);

	if(0 > sj_store_file(jsonf, file)) {
		printf("Could not diff info\n");
		return -1;
	}

	json_decref(jsonf);
	return 0;
}



/***********************************************************************
 * Function: extract_diff_info
 *
 * Called by create_diff_image for storage and retrieval of diff data.
 */
int extract_diff_info(char *dir, int *size, struct ecu_update_str *updts)
{
	json_t *jsonf;
	int fe;
	char file[JSON_NAME_SIZE];
	char ecu_name[JSON_NAME_SIZE];

	if((dir == NULL) || (updts == NULL))
		return -1;

	sprintf(file, "%s/%s.json", dir, updts->ecu_name);
	fe = access(file, F_OK);
	if(fe != 0) 
		return -1;

	if(0 > sj_load_file(file, &jsonf))
		return -1;

	sj_get_string(jsonf, "ecu_name", ecu_name);
	if(0 != strcmp(ecu_name, updts->ecu_name)) {
		json_decref(jsonf);
		return -1;
	}

	sj_get_int(jsonf, "original_size", size);
	sj_get_string(jsonf, "new_sha256", updts->new_sha256);

	json_decref(jsonf);
	return 0;
}



/***********************************************************************
 * Function: create_diff_image
 *
 * This function creates diff images
 *
 * return: -1 in case of errors
 *	   orig_size - size of base release version if all is well.
 */
int create_diff_image(char *vin, struct ecu_update_str *updts)
{
	int fe_c, fe_n, fe_d, len;
	int orig_size = 0;
	FILE *fp;
	char *pathc, *pathn, *diffe, *ecu;
	char cmd_buf[JSON_NAME_SIZE], *sp;
	char cachdir[JSON_NAME_SIZE];
	char cwd[JSON_NAME_SIZE];

	ecu = updts->ecu_name;
	pathc = updts->pathc;
	pathn = updts->pathn;
	diffe = updts->diff_engine;
	printf("Creating diff for \"%s\":\n", ecu);

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

	/* let's do some caching to reduce work later */
	get_cache_dir(vin, ecu, pathc, pathn, diffe, cachdir);
	fe_d = access(cachdir, F_OK);
	if(fe_d != 0) {
		create_dir(cachdir);
	}
	else {
		/* cache hit!! extract info and return */
		increment_cache_dir_usecount(cachdir);
		if(0 <= extract_diff_info(cachdir, &orig_size, updts))
			goto add_diff_to_tar;
	}

	/* copy release files */
	printf("  copying files...\n\t");
	update_sota_status(Client.id, "Server", "Copying release tarballs...");
	sprintf(cmd_buf, "cp -f %s %s/cur.tar.bz2", pathc, SessionPath);
	system(cmd_buf);
	printf("\t");
	sprintf(cmd_buf, "cp -f %s %s/new.tar.bz2", pathn, SessionPath);
	system(cmd_buf);

	/* clean up previous files from work area */
	printf("\t");
	sprintf(cmd_buf, "rm -f %s/*.tar", SessionPath);
	system(cmd_buf);

	/* base release file processing */
	printf("  decompressing base release file...\n\t");
	update_sota_status(Client.id, "Server",
			   "Decompressing base tar ball...");
	sprintf(cmd_buf, "bzip2 -d %s/cur.tar.bz2", SessionPath);
	system(cmd_buf);

	/* new release file processing */
	printf("  decompressing new release file...\n\t");
	update_sota_status(Client.id, "Server",
			   "Decompressing new tar ball...");
	sprintf(cmd_buf, "bzip2 -d %s/new.tar.bz2", SessionPath);
	system(cmd_buf);

	sprintf(cmd_buf, "%s/new.tar", SessionPath);
	orig_size = get_filesize(cmd_buf);
	if(orig_size < 0)
		return -1;

	printf("  computing sha256sum for the new file...\n\t");
	update_sota_status(Client.id, "Server",
			   "Computing sha256sum for the new image...");
	sprintf(cmd_buf, "sha256sum %s/new.tar > %s/new.sum",
		SessionPath, SessionPath);
	system(cmd_buf);

	/* capture the sha256 value to update structure */
	sprintf(cmd_buf, "%s/new.sum", SessionPath);
	if(0 > cut_sha256sum_fromfile(cmd_buf, updts->new_sha256,
				      JSON_NAME_SIZE))
		return -1;

	/* cache diff info for future use */
	store_diff_info(cachdir, orig_size, updts);

	/* find the delta */
	printf("  preparing diff file...\n\t");
	update_sota_status(Client.id, "Server", "Preparing diff file...");
	sprintf(cmd_buf, "%s %s/cur.tar %s/new.tar %s/%s_diff.tar",
		diffe, SessionPath, SessionPath, cachdir, ecu);
	system(cmd_buf);

	/* compress the diff file */
	printf("  compressing diff file...\n\t");
	update_sota_status(Client.id, "Server", "Compressing diff file...");
	sprintf(cmd_buf, "bzip2 -z %s/%s_diff.tar", cachdir, ecu);
	system(cmd_buf);

add_diff_to_tar:
	DownloadInfo.compression_type = SOTA_BZIP2;

	/* change dir to exclude dir information in the tar file */
	if(getcwd(cwd, sizeof(cwd)) == NULL)
		return -1;
	if(0 > chdir(cachdir))
		return -1;

	/* add the diff file to int.diff.tar */
	sprintf(cmd_buf, "tar uvf %s/int.diff.tar %s_diff.tar.bz2",
		SessionPath, ecu);
	printf("%s\n", cmd_buf);
	system(cmd_buf);

	/* change directory back to original */
	chdir(cwd);

	return orig_size;
}



/***********************************************************************
 * Function: any_ecu_needs_update
 *
 * This function checks all cur_version and new_versions of ecus in all
 * vehicle (vin) and return 1 if it needs update
 *
 * return: 0 - no update needed
 *        -1 - error
 *         1 - updated needed
 */
int any_ecu_needs_update(char *vin, char *tbl)
{
	return db_compare_columns(tbl, "cur_version", "new_version");
}



/*************************************************************************
 * Function: update_ecu_table
 *
 * This function is called as soon as a client successfully logged in to
 * the sota system. The function checks the current version of client and
 * update the database about the version number of the client.
 *
 * return 0 - if versions are same
 *	  1 - if update happened
 *	 -1 - for errors
 */
int update_ecu_table(char *tbl, char *vin, json_t *jsonf)
{
	json_t *jarray, *jrow;
	int i, ecus, ret;
	int update_happened = 0;
	struct ecu_info ei;
	char db_version[JSON_NAME_SIZE];

	if((tbl == NULL)||(*tbl == '\0')||(vin == NULL)||(jsonf == NULL)) {
		printf("%s(), invalid argument\n", __FUNCTION__);
		return -1;
	}

	/* check if the json file has array of ecu structure in it */
	jarray = json_object_get(jsonf, "ecus");
	if((jarray == NULL) || (0 == (ecus = json_array_size(jarray)))) {
		printf("%s(), no array element \'ecus\' found\n", __FUNCTION__);
		return -1;
	}
	printf("Number of ECUS: %d\n", ecus);

	for(i = 0; i < ecus; i++) {
		jrow = json_array_get(jarray, i);
		if(!json_is_object(jrow)) {
			printf("%s(), json array access failure\n",
			       __FUNCTION__);
			return -1;
		}

		/* extract all ecu info */
		sj_get_string(jrow, "ecu_name", ei.ecu_name);
		tolower_str(ei.ecu_name);
		sj_get_string(jrow, "ecu_make", ei.ecu_make);
		tolower_str(ei.ecu_make);
		sj_get_string(jrow, "sw_version", ei.sw_version);
		sj_get_string(jrow, "diff_tool", ei.diff_tool);
		sj_get_string(jrow, "patch_tool", ei.patch_tool);
		sj_get_string(jrow, "serial_no", ei.serial_no);
		sj_get_int(jrow, "year", &ei.year);

		/* check if version matches */
		ret = db_get_columnstr_fromkeystr(tbl, "cur_version",
				db_version, "ecu_name", ei.ecu_name);
		if(ret <= 0) {
			printf("%s(), can't get cur_version!\n", __FUNCTION__);
			return -1;
		}
		if(0 == strcmp(db_version, ei.sw_version))
			continue;

		/* version mis-match, update table */
		ret = db_set_columnstr_fromkeystr(tbl, "cur_version",
				ei.sw_version, "ecu_name", ei.ecu_name);
		if(ret <= 0) {
			printf("%s(), can't set cur_version!\n", __FUNCTION__);
			return -1;
		}

		/* check if client updated to new_version */
		ret = db_get_columnstr_fromkeystr(tbl, "new_version",
				db_version, "ecu_name", ei.ecu_name);
		if(ret <= 0) {
			printf("%s(), can't get new_version!\n", __FUNCTION__);
			return -1;
		}
		if(0 == strcmp(db_version, ei.sw_version))
			update_happened = 1;
	}

	json_decref(jarray);
	json_decref(jrow);

	/* restrict continuous update / downloads */
	if(update_happened) {
		/* mark the database as no more downloads allowed */
		ret = db_set_columnint_fromkeystr(SOTATBL_VEHICLE,
					 "allowed", 0, "vin", vin);
		if(ret <= 0) {
			printf("%s(), can't set allowed!\n", __FUNCTION__);
			return -1;
		}

		/* increment update count and update date */
		update_client_log(Client.id, "ucount", "udate");
	}

	return 0;
}



/*************************************************************************
 * Function: init_ecus_table_name
 *
 * This function will be called as soon as a success login even has 
 * happened. This function initializes a global variable 'ECU_Table',
 * which is used find the software version details of different ECUS.
 */
int init_ecus_table_name(char *vin)
{
	if(vin == NULL) {
		printf("%s(), invalid arguments\n", __FUNCTION__);
		return -1;
	}

	sprintf(ECU_Table, "ecus_%s_tbl", vin);
	return 0;
}



/*************************************************************************
 * Function: extract_ecus_info
 *
 * This function is called for the first time when the vehicle registers
 * for the first time. 
 *
 * This function extracts all the information related to the ECUs in the
 * vehicle from the client_info.json file.
 */
int extract_ecus_info(json_t *jsonf, char *vin)
{
	json_t *jarray;
	json_t *jrow;
	int i, ecus;
	struct ecu_info ecu_row;

	if(jsonf == NULL) {
		printf("%s(), invalid argument passed\n", __FUNCTION__);
		return -1;
	}

	/* check if the json file has array of ecu structure in it */
	jarray = json_object_get(jsonf, "ecus");
	if(!json_is_array(jarray)) {
		printf("Panic!! Could not get ecu array\n");
		return -1;
	}

	ecus = json_array_size(jarray);
	if(0 == ecus) {
		printf("%s(), no rows in \'ecus\' array\n", __FUNCTION__);
		return -1;
	}
	printf("Number of ECUS: %d\n", ecus);

	init_ecus_table_name(vin);
	if(0 < db_create_ecutbl_ifnotexist(ECU_Table))
		return -1;

	/* populate ECU info */
	for(i = 0; i < ecus; i++) {
		jrow = json_array_get(jarray, i);
		if(!json_is_object(jrow)) {
			printf("%s(), json array access failure\n",
			       __FUNCTION__);
			return -1;
		}

		sj_get_string(jrow, "ecu_name", ecu_row.ecu_name);
		tolower_str(ecu_row.ecu_name);
		sj_get_string(jrow, "ecu_make", ecu_row.ecu_make);
		tolower_str(ecu_row.ecu_make);
		sj_get_string(jrow, "diff_tool", ecu_row.diff_tool);
		sj_get_string(jrow, "patch_tool", ecu_row.patch_tool);
		sj_get_string(jrow, "serial_no", ecu_row.serial_no);
		sj_get_string(jrow, "sw_version", ecu_row.sw_version);
		sj_get_int(jrow, "year", &ecu_row.year);

		db_insert_ecutbl_row(ECU_Table, &ecu_row);
	}

	json_decref(jarray);
	return 0;
}
