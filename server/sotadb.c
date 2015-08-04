/* 
 * Author: Aananth C N
 * email: c.n.aananth@gmail.com
 *
 * License: GPL v2
 * Date: 14 July 2015
 */
#include <mysql/mysql.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sotadb.h"


#define QRYSIZE (128*1024)
#define STRSIZE (256)


MYSQL mysql;


/************************************************************************
 * Utility functions 
 */
void db_print_error(char *q)
{
	printf("Query: %s\n", q);
	printf("Error: %s\n", mysql_error(&mysql));
}


void db_print_table(void)
{
	MYSQL_RES *res;
	MYSQL_ROW row;

	/* send SQL query */
	if (mysql_query(&mysql, "show tables")) {
		db_print_error("show tables");
		return;
	}
	res = mysql_use_result(&mysql);

	/* output table name */
	printf("MySQL Tables in mysql database:\n");
	while ((row = mysql_fetch_row(res)) != NULL)
		printf("\t%s \n", row[0]);

	printf("\n  --end-of-table--\n");

	mysql_free_result(res);
}



/************************************************************************
 * Main functions 
 */
int db_insert_row(char *tbl, struct client_tbl_row *row)
{
	char query[QRYSIZE];

	if(tbl == NULL)
		return -1;

	snprintf(query, QRYSIZE, "INSERT INTO %s (vin, serial_no, name, phone, email, make, model, device, variant, year, cur_version) VALUES (\'%s\', \'%s\', \'%s\', \'%s\', \'%s\', \'%s\', \'%s\', \'%s\', \'%s\', \'%d\', \'%s\')",
		 tbl, row->vin, row->serial_no, row->name, row->phone,
		 row->email, row->make, row->model, row->device,
		 row->variant, row->year, row->cur_sw_version);

	if(0 != mysql_query(&mysql, query)) {
		db_print_error(query);
		return -1;
	}
	else {
		printf("Adding row to \"%s\"...\n", tbl);
		return 0;
	}

	return 0;
}



/************************************************************************
 * Function: db_get_sjrow_colmatchstr
 *
 * This function search a column with name passed in arg3 in sotatbl and
 * find a match to the arg3 and copy the row in a json file passed in arg2
 *
 * arg1: Name of the MYSQL table
 * arg2: column name of MYSQL table
 * arg3: value to be searched in column
 * arg4: json file with contents of searched row to be copied
 *
 * return: return value has 2 different meanings
 *         '< 0'  - Error
 *         '>= 0' - Success
 */
int db_get_sjrow_colmatchstr(char *tbl, char *col, char *value, char *jfile)
{
	unsigned int num_fields;
	unsigned int i, ret = 0;
	MYSQL_FIELD *fields;
	MYSQL_RES *res;
	MYSQL_ROW sqlrow;
	char query[QRYSIZE];
	char *plc;
	FILE *fp;

	if(tbl == NULL)
		return -1;

	snprintf(query, QRYSIZE, "SELECT * FROM %s.%s WHERE BINARY %s=\'%s\'",
		 SOTADB_DBNAME, tbl, col, value);

	if(0 != mysql_query(&mysql, query)) {
		db_print_error(query);
		return -1;
	}
	else {
		printf("%s\n", query);
		res = mysql_use_result(&mysql);
		if(res == NULL) {
			db_print_error("use result call");
			return -1;
		}

		num_fields = mysql_num_fields(res);
		fields = mysql_fetch_fields(res);
		sqlrow = mysql_fetch_row(res);
		if((fields == NULL) || (sqlrow == NULL))
			return -1;

		fp = fopen(jfile, "w");
		if(fp == NULL) {
			printf("Could not open %s\n", jfile);
			return -1;
		}

		fprintf(fp, "{\n");
		for(i = 0; i < num_fields; i++)
		{
			if(sqlrow[i] == NULL) {
				ret = -1;
				break;
			}

			if(i == (num_fields-1))
				plc = "";
			else
				plc = ",";

			if(fields[i].type == MYSQL_TYPE_LONG)
				fprintf(fp, "\t\"%s\": %s%s\n",
					fields[i].name, sqlrow[i], plc);
			else
				fprintf(fp, "\t\"%s\": \"%s\"%s\n",
					fields[i].name, sqlrow[i], plc);
		}
		fprintf(fp, "}\n");
		fclose(fp);

		mysql_free_result(res);
		return ret;
	}

	return -1;
}



/************************************************************************
 * Function: db_get_columnstr_fromkeyint
 *
 * This function search a column string from a integer key passed in last
 * two arguments and finally it copies the string to arg3 
 *
 * arg1: Name of the MYSQL table
 * arg2: column name of MYSQL table
 * arg3: return string will be copied to this argument
 * arg4: name of the key column
 * arg5: key value
 *
 * return: return value has 3 different meanings
 *         '< 0' - Error
 *         '> 0' - search success
 *         '= 0' - search failed
 */
int db_get_columnstr_fromkeyint(char *tbl, char *col, char *cval,
			     char* key, int kval)
{
	char query[QRYSIZE];
	MYSQL_RES *res;
	MYSQL_ROW row;
	int ret = 0;

	if(tbl == NULL)
		return -1;

	snprintf(query, QRYSIZE, "SELECT %s FROM %s.%s WHERE BINARY %s=\'%d\'",
		 col, SOTADB_DBNAME, tbl, key, kval);

	if(0 != mysql_query(&mysql, query)) {
		db_print_error(query);
		return -1;
	}

	res = mysql_use_result(&mysql);
	if(res == NULL)
		db_print_error("use result call");

	/* search if the vin matches with any in table */
	while ((row = mysql_fetch_row(res)) != NULL) {
		if(row[0] == NULL) {
			*cval = '\0';
			ret = 0;
		}
		else {
			strcpy(cval, row[0]);
			ret = 1;
		}
		break;
	}

	mysql_free_result(res);
	return ret;
}




/************************************************************************
 * Function: db_get_columnstr_fromkeystr
 *
 * This function search a column with name passed in arg2 in sotatbl and 
 * copies the string in that column match to arg3 
 *
 * arg1: Name of the MYSQL table
 * arg2: column name of MYSQL table
 * arg3: value to be searched in column
 *
 * return: return value has 3 different meanings
 *         '< 0' - Error
 *         '> 0' - search success
 *         '= 0' - search failed
 */
int db_get_columnstr_fromkeystr(char *tbl, char *col, char *cval,
			     char* key, char *kval)
{
	char query[QRYSIZE];
	MYSQL_RES *res;
	MYSQL_ROW row;
	int ret = 0;

	if(tbl == NULL)
		return -1;

	snprintf(query, QRYSIZE, "SELECT %s FROM %s.%s WHERE BINARY %s=\'%s\'",
		 col, SOTADB_DBNAME, tbl, key, kval);

	if(0 != mysql_query(&mysql, query)) {
		db_print_error(query);
		return -1;
	}

	res = mysql_use_result(&mysql);
	if(res == NULL)
		db_print_error("use result call");

	/* search if the vin matches with any in table */
	while ((row = mysql_fetch_row(res)) != NULL) {
		if(row[0] == NULL) {
			*cval = '\0';
			ret = 0;
		}
		else {
			strcpy(cval, row[0]);
			ret = 1;
		}
		break;
	}

	mysql_free_result(res);
	return ret;
}


/************************************************************************
 * Function: db_check_col_str
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
int db_check_col_str(char *tbl, char *col, char *value)
{
	int ret;
	char dbval[STRSIZE];

	ret = db_get_columnstr_fromkeystr(tbl, col, dbval, col, value);

	if(ret > 0) {
		if(0 != strcmp(dbval, value)) {
			ret = 0;
		}
	}

	return ret;
}



/************************************************************************
 * Function: db_set_columnint_fromkeyint
 *
 * This function modifies a column with name passed in arg2 in sotatbl and
 * with the value passed in the 3rd argument
 *
 * arg1: Name of the MYSQL table
 * arg2: column name of MYSQL table
 * arg3: value to be written to a row searched by 4th and 5th arg
 * arg4: key string
 * arg5: key value
 *
 * return: return value has 3 different meanings
 *         '< 0' - Error
 *         '> 0' - successfully updated the column, row 
 *         '= 0' - search failed
 */
int db_set_columnint_fromkeyint(char *tbl, char *col, int cval,
			     char *key, int kval)
{
	char query[QRYSIZE];
	MYSQL_RES *res;
	MYSQL_ROW row;
	int ret = 0;

	if(tbl == NULL)
		return -1;

	snprintf(query, QRYSIZE, "UPDATE %s.%s SET %s=\'%d\' WHERE %s=\'%d\'",
		 SOTADB_DBNAME, tbl, col, cval, key, kval);

	if(0 != mysql_query(&mysql, query)) {
		db_print_error(query);
		return -1;
	}

	if(*mysql_error(&mysql))
	{
		/* error occurred */
		printf("QUERY: %s\n", query);
		ret = 0;
	}
	else {
		ret = 1;
	}

	return ret;
}




/************************************************************************
 * Function: db_set_columnstr_fromkeyint
 *
 * This function modifies a column with name passed in arg2 in sotatbl and
 * with the value passed in the 3rd argument
 *
 * arg1: Name of the MYSQL table
 * arg2: column name of MYSQL table
 * arg3: value to be written to a row searched by 4th and 5th arg
 * arg4: key string
 * arg5: key value
 *
 * return: return value has 3 different meanings
 *         '< 0' - Error
 *         '> 0' - successfully updated the column, row 
 *         '= 0' - search failed
 */
int db_set_columnstr_fromkeyint(char *tbl, char *col, char *cval,
			     char *key, int kval)
{
	char query[QRYSIZE];
	MYSQL_RES *res;
	MYSQL_ROW row;
	int ret = 0;

	if(tbl == NULL)
		return -1;

	snprintf(query, QRYSIZE, "UPDATE %s.%s SET %s=\'%s\' WHERE %s=\'%d\'",
		 SOTADB_DBNAME, tbl, col, cval, key, kval);

	if(0 != mysql_query(&mysql, query)) {
		db_print_error(query);
		return -1;
	}

	if(*mysql_error(&mysql))
	{
		/* error occurred */
		printf("QUERY: %s\n", query);
		ret = 0;
	}
	else {
		ret = 1;
	}

	return ret;
}




/************************************************************************
 * Function: db_get_columnint_fromkeystr
 *
 * This function search a column with name passed in arg2 in sotatbl and 
 * copies into the arg3 passed 
 *
 * arg1: Name of the MYSQL table
 * arg2: column name of MYSQL table
 * arg3: value returned back to the caller
 *
 * return: return value has 3 different meanings
 *         '< 0' - Error
 *         '> 0' - search passed, row already exists
 *         '= 0' - search failed, new entry
 */
int db_get_columnint_fromkeystr(char *tbl, char *col, int *cval,
			     char *key, char *kval)
{
	char query[QRYSIZE];
	MYSQL_RES *res;
	MYSQL_ROW row;
	int ret = 0;

	if(tbl == NULL)
		return -1;

	snprintf(query, QRYSIZE, "SELECT %s FROM %s.%s WHERE BINARY %s=\'%s\'",
		 col, SOTADB_DBNAME, tbl, key, kval);

	if(0 != mysql_query(&mysql, query)) {
		db_print_error(query);
		return -1;
	}

	res = mysql_use_result(&mysql);
	if(res == NULL)
		db_print_error("use result call");

	/* search if the vin matches with any in table */
	while ((row = mysql_fetch_row(res)) != NULL) {
		if(row[0] != NULL) {
			*cval = strtol(row[0], NULL, 10);
			ret = 1;
		}
		else {
			*cval = 0;
			ret = 0;
		}
		break;
	}

	mysql_free_result(res);
	return ret;
}


/************************************************************************
 * Function: db_get_columnint_fromkeyint
 *
 * This function search a column with name passed in arg2 in sotatbl and 
 * copies into the arg3 passed 
 *
 * arg1: Name of the MYSQL table
 * arg2: column name of MYSQL table
 * arg3: value returned back to the caller
 *
 * return: return value has 3 different meanings
 *         '< 0' - Error
 *         '> 0' - Check passed, row already exists
 *         '= 0' - Check failed, new entry
 */
int db_get_columnint_fromkeyint(char *tbl, char *col, int *cval,
			     char *key, int kval)
{
	char query[QRYSIZE];
	MYSQL_RES *res;
	MYSQL_ROW row;
	int ret = 0;

	if(tbl == NULL)
		return -1;

	snprintf(query, QRYSIZE, "SELECT %s FROM %s.%s WHERE BINARY %s=\'%d\'",
		 col, SOTADB_DBNAME, tbl, key, kval);

	if(0 != mysql_query(&mysql, query)) {
		db_print_error(query);
		return -1;
	}

	res = mysql_use_result(&mysql);
	if(res == NULL)
		db_print_error("use result call");

	/* search if the vin matches with any in table */
	while ((row = mysql_fetch_row(res)) != NULL) {
		if(row[0] != NULL)
			*cval = strtol(row[0], NULL, 10);
		else
			*cval = 0;
		ret = 1;
		break;
	}

	mysql_free_result(res);
	return ret;
}


/************************************************************************
 * Function: db_check_col_int
 *
 * This function search a column with name passed in arg2 in sotatbl and 
 * checks if any of the string in that column match with arg3 
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
int db_check_col_int(char *tbl, char *col, int value)
{
	int dbval;
	int ret = 0;

	if(tbl == NULL)
		return -1;

	ret = db_get_columnint_fromkeyint(tbl, col, &dbval, col, value);

	if(ret > 0) {
		if(dbval != value)
			ret = 0;
	}

	return ret;
}

int db_check_and_create_table(char *tbl)
{
	char query[QRYSIZE];
	MYSQL_RES *res;

	if(tbl == NULL)
		return -1;

	/* check if table exists */
	snprintf(query, QRYSIZE, "SELECT id FROM %s.%s", SOTADB_DBNAME, tbl);
	if(0 != mysql_query(&mysql, query)) {
		/* table not found */
		goto create_table;
	}

	/* lets double confirm */
	res = mysql_use_result(&mysql);
	if(res != NULL) {
		/* table exists */
		mysql_free_result(res);
		return 0;
	}
	else {
		/* table not found */
		mysql_free_result(res);
		goto create_table;
	}

	return 0;

create_table:
	snprintf(query, QRYSIZE, "CREATE TABLE IF NOT EXISTS %s.%s ( id int not null auto_increment,  vin varchar(255), serial_no varchar(255), name varchar(255), phone varchar(255), email varchar(255), make varchar(255), model varchar(255), device varchar(255), variant varchar(255), year int, cur_version varchar(255), new_version varchar(255), allowed int,  PRIMARY KEY (id, vin) )", SOTADB_DBNAME, tbl);

	printf("Creating table \"%s\"...\n", tbl);
	if(0 != mysql_query(&mysql, query)) {
		db_print_error(query);
		return -1;
	}
}


int db_close(void)
{
	mysql_close(&mysql);
	mysql_library_end();
	return 0;
}


int db_init(void)
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
			db_print_error(query);
			db_close();
			return -1;
		}
		else {
			printf("Creating database for the first time ...\n");
		}
	}

	db_check_and_create_table(SOTATBL_VEHICLE);
	db_print_table();
}
