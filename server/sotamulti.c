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


struct sota_ecus *ECUp[];
int ECUs;


int extract_ecus_info(char *ifile)
{
	json_t *jsonf, *jarray, *jrow;
	int fe, i;

	fe = access(ifile, F_OK);
	if(fe != 0) {
		printf("%s(), can't open file %s\n", __FUNCTION__, ifile);
		return -1;
	}

	if(0 > sj_load_file(ifile, &jsonf))
		return -1;

	/* check if the json file has array of ecu structure in it */
	jarray = json_object_get(jsonf, "ecus");
	if((jarray == NULL) || (0 == (ECUs = json_array_size(jarray)))) {
		printf("%s(), no array element \'ecus\' found\n", __FUNCTION__);
		return -1;
	}
	printf("Number of ECUS: %d\n", ECUs);

	/* allocate memory for SOTA ECU info */
	ECUp = (struct sota_ecus **) malloc(ECUs * sizeof(struct sota_ecus));
	if(ECUp == NULL) {
		printf("%s(), memory allocation failure\n", __FUNCTION__);
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

		sj_get_string(jrow, "ecu_name", ECUp[i]->device);
		sj_get_string(jrow, "diff_engine", ECUp[i]->diff_engine);
		sj_get_string(jrow, "serial_no", ECUp[i]->serial_no);
		sj_get_string(jrow, "make", ECUp[i]->make);
		sj_get_string(jrow, "model", ECUp[i]->model);
		sj_get_string(jrow, "variant", ECUp[i]->variant);
		sj_get_string(jrow, "sw_version", ECUp[i]->sw_version);
		sj_get_int(jrow, "year", &ECUp[i]->year);
	}
}
