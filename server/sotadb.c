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


#define QRYSIZE (128*1024)


MYSQL mysql;


/************************************************************************
 * Utility functions 
 */
int sotadb_print_error(void)
{
	printf("MYSQL Error: %s\n", mysql_error(&mysql));
}

int sotadb_print_table(void)
{
	MYSQL_RES *res;
	MYSQL_ROW row;

	/* send SQL query */
	if (mysql_query(&mysql, "show tables")) {
		sotadb_print_error();
		return -1;
	}
	res = mysql_use_result(&mysql);

	/* output table name */
	printf("MySQL Tables in mysql database:\n");
	while ((row = mysql_fetch_row(res)) != NULL)
		printf("%s \n", row[0]);

	printf("\n    --end-of-table--\n");
}



/************************************************************************
 * Main functions 
 */
int sotadb_add_row(char *tbl, struct client_tbl_row *row)
{
	char query[QRYSIZE];

	if(tbl == NULL)
		return -1;

	sotadb_print_table();
	snprintf(query, QRYSIZE, "INSERT INTO %s (vin, serial_no, name, phone, email, make, model, device, variant, year) VALUES (\'%s\', \'%s\', \'%s\', \'%s\', \'%s\', \'%s\', \'%s\', \'%s\', \'%s\', \'%d\')",
		 tbl, row->vin, row->serial_no, row->name, row->phone, 
		 row->email, row->make, row->model, row->device, 
		 row->variant, row->year);

	if(0 == mysql_query(&mysql, query)) {
		printf("Adding row to \"%s\"...\n", tbl);
		return 0;
	}
	else {
		sotadb_print_error();
		printf("Could not insert row to \"%s\"\n", tbl);
		printf("%s\n", query);
		return -1;
	}
	sotadb_print_table();

	return 0;
}

int sotadb_check_and_create_table(char *tbl)
{
	char query[QRYSIZE];

	if(tbl == NULL)
		return -1;

	snprintf(query, QRYSIZE, "CREATE TABLE IF NOT EXISTS %s.%s ( id int not null auto_increment,  vin varchar(255), serial_no varchar(255), name varchar(255), phone varchar(255), email varchar(255), make varchar(255), model varchar(255), device varchar(255), variant varchar(255), year smallint, cur_sw_version varchar(255), new_sw_version varchar(255), update_available smallint,  PRIMARY KEY (id, vin) )", SOTADB_DBNAME, tbl);
	if(0 == mysql_query(&mysql, query)) {
		printf("Creating table \"%s\"...\n", tbl);
		return 0;
	}
	else {
		sotadb_print_error();
		printf("%s\n", query);
	}

	return 0;
}


int init_sotadb(void)
{
	MYSQL *rdb;
	char query[QRYSIZE];

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
		snprintf(query, QRYSIZE, "CREATE DATABASE %s", SOTADB_DBNAME);
		if(0 == mysql_query(&mysql, query))
			printf("Creating database for the first time ...\n");
		else {
			printf("Unable to create database \"%s\"\n",
			       SOTADB_DBNAME);
			close_sotadb();
			return -1;
		}
	}

	sotadb_check_and_create_table(SOTATBL_VEHICLE);
	sotadb_print_table();
}

int close_sotadb(void)
{
	mysql_close(&mysql);
	mysql_library_end();
	return 0;
}
