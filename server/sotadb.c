/* 
 * Author: Aananth C N
 * email: caananth@visteon.com
 *
 * License: GPL v2
 * Date: 14 July 2015
 */
#include <mysql/mysql.h>
#include <stdlib.h>
#include <stdio.h>

#include "sotadb.h"

#define SOTADB_QUERY "SELECT SCHEMA_NAME FROM INFORMATION_SCHEMA.SCHEMATA WHERE SCHEMA_NAME = 'sotadb'"

MYSQL mysql;

int init_sotadb(void)
{
	MYSQL *rdb;

	if(mysql_library_init(0, NULL, NULL)) {
		printf("could not initialize MySQL library\n");
		return -1;
	}

	mysql_init(&mysql);

	rdb = mysql_real_connect(&mysql, SOTADB_SERVER, SOTADB_USERNM,
			   SOTADB_PASSWD, NULL, 0, NULL, 0);

	if(rdb == NULL) {
		printf("Failed to connect to database: Error: %s\n",
			mysql_error(&mysql));
		mysql_library_end();
		return -1;
	}

	if(0 != mysql_select_db(&mysql, SOTADB_DBNAME)) {
		if(0 == mysql_query(&mysql, SOTADB_CREATE))
			printf("Creating database for the first time ...\n");
		else {
			printf("Unable to create database \"%s\"\n",
			       SOTADB_DBNAME);
			close_sotadb();
			return -1;
		}
	}

	return 0;
}

int close_sotadb(void)
{
	mysql_close(&mysql);
	mysql_library_end();
	return 0;
}
