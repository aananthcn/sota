/* 
 * Author: Aananth C N
 * email: c.n.aananth@gmail.com
 *
 * License: GPL v2
 * Date: 28 August 2015
 */
#ifndef SOTACMULTI_H
#define SOTACMULTI_H

#include "sotajson.h"

struct uinfo {
	char ecu_name[JSON_NAME_SIZE];
	char new_version[JSON_NAME_SIZE];
	char new_sha256[JSON_NAME_SIZE];
	int update_available;
};

extern int ECUs;
extern struct ecu_info *ECU_Info;


int extract_ecus_info(json_t *jsonf);


#endif
