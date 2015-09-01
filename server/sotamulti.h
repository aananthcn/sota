/* 
 * Author: Aananth C N
 * email: c.n.aananth@gmail.com
 *
 * License: GPL v2
 * Date: 26 August 2015
 */
#ifndef SOTASMULTI_H
#define SOTASMULTI_H

#include "sotajson.h"


struct ecu_update_str {
	char ecu_name[JSON_NAME_SIZE];
	char cur_version[JSON_NAME_SIZE];
	char new_version[JSON_NAME_SIZE];
	char new_sha256[JSON_NAME_SIZE];
	char pathn[JSON_NAME_SIZE];
	char pathc[JSON_NAME_SIZE];
	char diff_engine[JSON_NAME_SIZE];
	int update_available;
};

extern char ECU_Table[];
int extract_ecus_info(json_t *jsonf, char *vin);

#endif
