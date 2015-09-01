/* 
 * Author: Aananth C N
 * email: c.n.aananth@gmail.com
 *
 * License: GPL v2
 * Date: 28 August 2015
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
#include "sotacommon.h"
#include "sotamulti.h"


int ECUs;
struct ecu_info *ECU_Info;



/*************************************************************************
 * Function: extract_x_from_uinfo
 *
 * This function extracts the information passed by first argument for a
 * matching ecu_name passed as 2nd argument on a file passed as 3rd
 * argument.
 *
 * return: searched field is returned via 4th arg. 
 *	   1 - search found
 *	   0 - not found
 *	  -1 - error
 */
int extract_x_from_uinfo(char *field, char *name, char *jfile, char *val)
{
	json_t *jsonf, *jarray, *jrow;
	int i, rows;
	int result = 0;
	char ecu_name[JSON_NAME_SIZE];

	if((jfile == NULL) || (field == NULL) || (ecu_name == NULL)) {
		printf("%s(), invalid argument passed\n", __FUNCTION__);
		return -1;
	}

	/* load the json file to extract the field */
	if(0 > sj_load_file(jfile, &jsonf))
		return -1;

	/* check if the json file has array of ecu structure in it */
	jarray = json_object_get(jsonf, "ecus");
	if((jarray == NULL) || (0 == (rows = json_array_size(jarray)))) {
		printf("%s(), no array element \'ecus\' found\n", __FUNCTION__);
		return -1;
	}

	/* search ecu_name */
	for(i = 0; i < rows; i++) {
		jrow = json_array_get(jarray, i);
		if(!json_is_object(jrow)) {
			printf("%s(), json array access failure\n",
			       __FUNCTION__);
			return -1;
		}

		sj_get_string(jrow, "ecu_name", ecu_name);
		if(0 == strcmp(ecu_name, name)) {
			sj_get_string(jrow, field, val);
			result = 1;
			break;
		}
	}

	json_decref(jarray);
	json_decref(jsonf);
	json_decref(jrow);
	return result;
}



/*************************************************************************
 * Function: extract_ecu_update_info
 *
 * This function extracts the information sent by server for its update
 * request to uinfo structure passed as first argument.
 */
int extract_ecu_update_info(struct uinfo *ui, int ecus, json_t *jsonf)
{
	json_t *jarray, *jrow;
	int i, rows;

	if(jsonf == NULL) {
		printf("%s(), invalid argument passed\n", __FUNCTION__);
		return -1;
	}

	/* check if the json file has array of ecu structure in it */
	jarray = json_object_get(jsonf, "ecus");
	if((jarray == NULL) || (0 == (rows = json_array_size(jarray)))) {
		printf("%s(), no array element \'ecus\' found\n", __FUNCTION__);
		return -1;
	}

	if(rows != ecus) {
		printf("%s(), rows != ecus\n", __FUNCTION__);
		return -1;
	}

	/* populate ECU info */
	for(i = 0; i < ecus; i++) {
		jrow = json_array_get(jarray, i);
		if(!json_is_object(jrow)) {
			printf("%s(), json array access failure\n",
			       __FUNCTION__);
			return -1;
		}

		sj_get_string(jrow, "ecu_name", ui[i].ecu_name);
		tolower_str(ui[i].ecu_name);
		sj_get_string(jrow, "new_version", ui[i].new_version);
		sj_get_string(jrow, "sha256sum_full", ui[i].new_sha256);
		sj_get_int(jrow, "update_available", &ui[i].update_available);
	}

	json_decref(jarray);
	json_decref(jrow);
	return 0;
}


/*************************************************************************
 * Function: add_multi_ecu_info
 *
 * This function appends multi ecu array to the json file object passed as
 * argument.
 */
int add_multi_ecu_info(json_t *jsonf)
{
	int i;
	json_t *jarray;

	if(jsonf == NULL) {
		printf("%s(), invalid argument passed\n", __FUNCTION__);
		return -1;
	}

	jarray = json_array();
	if(jarray == NULL) {
		printf("%s(), can't create array!\n", __FUNCTION__);
		return -1;
	}
	json_object_set(jsonf, "ecus", jarray);

	for(i = 0; i < ECUs; i++) {
		json_t *jrow;

		jrow = json_object();

		if(0 > sj_add_string(&jrow, "ecu_name",
				     ECU_Info[i].ecu_name)) {
			printf("%s() Can't add ecu_name\n", __FUNCTION__);
			return -1;
		}
		if(0 > sj_add_string(&jrow, "ecu_make",
				     ECU_Info[i].ecu_make)) {
			printf("%s() Can't add ecu_make\n", __FUNCTION__);
			return -1;
		}
		if(0 > sj_add_string(&jrow, "sw_version",
				     ECU_Info[i].sw_version)) {
			printf("%s() Can't add sw_version\n", __FUNCTION__);
			return -1;
		}
		if(0 > sj_add_string(&jrow, "diff_tool",
				     ECU_Info[i].diff_tool)) {
			printf("%s() Can't add diff_tool\n", __FUNCTION__);
			return -1;
		}
		if(0 > sj_add_string(&jrow, "patch_tool",
				     ECU_Info[i].patch_tool)) {
			printf("%s() Can't add patch_tool\n", __FUNCTION__);
			return -1;
		}
		if(0 > sj_add_string(&jrow, "serial_no",
				     ECU_Info[i].serial_no)) {
			printf("%s() Can't add serial_no\n", __FUNCTION__);
			return -1;
		}
		if(0 > sj_add_int(&jrow, "year", ECU_Info[i].year)) {
			printf("%s() Can't add ECU_Info\n", __FUNCTION__);
			return -1;
		}

		json_array_append(jarray, jrow);
		json_decref(jrow);
	}

	json_decref(jarray);
	return 0;
}



/*************************************************************************
 * Function: cleanup_multi_ecu_info
 *
 * This function free memory allocated for processing multi ECU update
 *
 * return: void
 */
void cleanup_multi_ecu_info(void)
{
	if(ECU_Info)
		free(ECU_Info);
}


/*************************************************************************
 * Function: extract_ecus_info
 *
 * This function extracts all the information related to the ECUs in the
 * vehicle from the client_info.json file.
 */
int extract_ecus_info(json_t *jsonf)
{
	json_t *jarray, *jrow;
	int i;

	if(jsonf == NULL) {
		printf("%s(), invalid argument passed\n", __FUNCTION__);
		return -1;
	}

	/* check if the json file has array of ecu structure in it */
	jarray = json_object_get(jsonf, "ecus");
	if((jarray == NULL) || (0 == (ECUs = json_array_size(jarray)))) {
		printf("%s(), no array element \'ecus\' found\n", __FUNCTION__);
		return -1;
	}
	printf("Number of ECUS: %d\n", ECUs);
	ECU_Info = malloc(ECUs * sizeof(struct ecu_info));
	if(ECU_Info == NULL) {
		printf("%s(), malloc failed\n", __FUNCTION__);
		return -1;
	}

	/* populate ECU info */
	for(i = 0; i < ECUs; i++) {
		jrow = json_array_get(jarray, i);
		if(!json_is_object(jrow)) {
			printf("%s(), json array access failure\n",
			       __FUNCTION__);
			return -1;
		}

		sj_get_string(jrow, "ecu_name", ECU_Info[i].ecu_name);
		tolower_str(ECU_Info[i].ecu_name);
		sj_get_string(jrow, "ecu_make", ECU_Info[i].ecu_make);
		tolower_str(ECU_Info[i].ecu_make);
		sj_get_string(jrow, "sw_version", ECU_Info[i].sw_version);
		sj_get_string(jrow, "diff_tool", ECU_Info[i].diff_tool);
		sj_get_string(jrow, "patch_tool", ECU_Info[i].patch_tool);
		sj_get_string(jrow, "serial_no", ECU_Info[i].serial_no);
		sj_get_int(jrow, "year", &ECU_Info[i].year);
	}

	json_decref(jarray);
	json_decref(jrow);
	return 0;
}
