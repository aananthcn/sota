/* 
 * Author: Aananth C N
 * email: c.n.aananth@gmail.com
 *
 * License: GPL v2
 * Date: 26 August 2015
 */
#ifndef SOTASERVER_H
#define SOTASERVER_H

#include "sotajson.h"


struct sota_ecu {
	char device[JSON_NAME_SIZE]; /* such Head Unit, Cluster etc */
	char serial_no[JSON_NAME_SIZE];
	char make[JSON_NAME_SIZE];
	char model[JSON_NAME_SIZE];
	char variant[JSON_NAME_SIZE];
	char sw_version[JSON_NAME_SIZE];
	int year;
	char diff_engine[JSON_NAME_SIZE];
};

extern struct sota_ecus *ECUp[];
extern int ECUs;


int extract_ecus_info(char *ifile);
