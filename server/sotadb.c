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
void sotadb_print_error(char *q)
{
	printf("Query: %s\n", q);
	printf("Error: %s\n", mysql_error(&mysql));
}


void sotadb_print_table(void)
{
	MYSQL_RES *res;
	MYSQL_ROW row;

	/* send SQL query */
	if (mysql_query(&mysql, "show tables")) {
		sotadb_print_error("show tables");
		return;
	}
	res = mysql_use_result(&mysql);

	/* output table name */
	printf("MySQL Tables in mysql database:\n");
	while ((row = mysql_fetch_row(res)) != NULL)
		printf("%s \n", row[0]);

	printf("\n    --end-of-table--\n");

	mysql_free_result(res);
}



/************************************************************************
 * Main functions 
 */
int sotadb_add_row(char *tbl, struct client_tbl_row *row)
{
	char query[QRYSIZE];

	if(tbl == NULL)
		return -1;

	snprintf(query, QRYSIZE, "INSERT INTO %s (vin, serial_no, name, phone, email, make, model, device, variant, year) VALUES (\'%s\', \'%s\', \'%s\', \'%s\', \'%s\', \'%s\', \'%s\', \'%s\', \'%s\', \'%d\')",
		 tbl, row->vin, row->serial_no, row->name, row->phone,
		 row->email, row->make, row->model, row->device,
		 row->variant, row->year);

	if(0 != mysql_query(&mysql, query)) {
		sotadb_print_error(query);
		return -1;
	}
	else {
		printf("Adding row to \"%s\"...\n", tbl);
		return 0;
	}

	return 0;
}


/************************************************************************
 * Function: sotadb_search_column_str
 *
 * This function search a column with name passed in arg2 in sotatbl and 
 * check if any of the string in that column match with arg3 
 *
 * arg1: Name of the MYSQL table
 * arg2: column name of MYSQL table
 * arg3: value to be searched in column
 *
 * return: return value has 3 different meanings
 *         '< 0' - Error
 *         '> 0' - Check passed, row already exists
 *         '= 0' - Check failed, new entry
 */
int sotadb_search_column_str(char *tbl, char *column, char *value)
{
	char query[QRYSIZE];
	MYSQL_RES *res;
	MYSQL_ROW row;
	int ret = 0;

	if(tbl == NULL)
		return -1;

	snprintf(query, QRYSIZE, "SELECT %s FROM %s.%s WHERE BINARY %s=\'%s\'",
		 column, SOTADB_DBNAME, tbl, column, value);

	if(0 != mysql_query(&mysql, query)) {
		sotadb_print_error(query);
		return -1;
	}

	res = mysql_use_result(&mysql);
	if(res == NULL)
		sotadb_print_error("use result call");

	/* search if the vin matches with any in table */
	while ((row = mysql_fetch_row(res)) != NULL) {
		if(0 == strcmp(row[0], value)) {
			ret = 1;
		}
	}

	mysql_free_result(res);
	return ret;
}


/************************************************************************
 * Function: sotadb_search_column_int
 *
 * This function search a column with name passed in arg2 in sotatbl and 
 * check if any of the string in that column match with arg3 
 *
 * arg1: Name of the MYSQL table
 * arg2: column name of MYSQL table
 * arg3: value to be searched in column
 *
 * return: return value has 3 different meanings
 *         '< 0' - Error
 *         '> 0' - Check passed, row already exists
 *         '= 0' - Check failed, new entry
 */
int sotadb_search_column_int(char *tbl, char *column, int value)
{
	char query[QRYSIZE];
	MYSQL_RES *res;
	MYSQL_ROW row;
	int ret = 0;

	if(tbl == NULL)
		return -1;

	snprintf(query, QRYSIZE, "SELECT %s FROM %s.%s WHERE BINARY %s=\'%d\'",
		 column, SOTADB_DBNAME, tbl, column, value);

	if(0 != mysql_query(&mysql, query)) {
		sotadb_print_error(query);
		return -1;
	}

	res = mysql_use_result(&mysql);
	if(res == NULL)
		sotadb_print_error("use result call");

	/* search if the vin matches with any in table */
	while ((row = mysql_fetch_row(res)) != NULL) {
		if(atoi(row[0]) == value) {
			ret = 1;
		}
	}

	mysql_free_result(res);
	return ret;
}

int sotadb_check_and_create_table(char *tbl)
{
	char query[QRYSIZE];

	if(tbl == NULL)
		return -1;

	snprintf(query, QRYSIZE, "CREATE TABLE IF NOT EXISTS %s.%s ( id int not null auto_increment,  vin varchar(255), serial_no varchar(255), name varchar(255), phone varchar(255), email varchar(255), make varchar(255), model varchar(255), device varchar(255), variant varchar(255), year smallint, cur_sw_version varchar(255), new_sw_version varchar(255), update_available smallint,  PRIMARY KEY (id, vin) )", SOTADB_DBNAME, tbl);

	if(0 != mysql_query(&mysql, query)) {
		sotadb_print_error(query);
		return -1;
	}

	printf("Creating table \"%s\"...\n", tbl);
	return 0;
}


int close_sotadb(void)
{
	mysql_close(&mysql);
	mysql_library_end();
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
		/* create sota database if not exist */
		snprintf(query, QRYSIZE, "CREATE DATABASE %s", SOTADB_DBNAME);
		if(0 != mysql_query(&mysql, query)) {
			sotadb_print_error(query);
			close_sotadb();
			return -1;
		}
		else {
			printf("Creating database for the first time ...\n");
		}
	}

	sotadb_check_and_create_table(SOTATBL_VEHICLE);
	sotadb_print_table();
}
