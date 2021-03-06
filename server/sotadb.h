/* 
 * Author: Aananth C N
 * email: c.n.aananth@gmail.com
 *
 * License: GPL v2
 * Date: 14 July 2015
 */
#ifndef SOTADB_H
#define SOTADB_H

#include <mysql/mysql.h>

#define SOTADB_SERVER	"localhost"
#define SOTADB_USERNM	"sota"
#define SOTADB_PASSWD	"visteon123"
#define SOTADB_DBNAME	"sotadb"
#define SOTADB_MAXCHAR	256

#define SOTATBL_VEHICLE	"sotatbl"
//#define SOTATBL_SWRELES "swreleasetbl"

struct sotatbl_row {
	int id;
	char vin[SOTADB_MAXCHAR];
	char name[SOTADB_MAXCHAR];
	char phone[SOTADB_MAXCHAR];
	char email[SOTADB_MAXCHAR];
	char make[SOTADB_MAXCHAR];
	char model[SOTADB_MAXCHAR];
	char variant[SOTADB_MAXCHAR];
	int year;
	int update_available;
	char state[SOTADB_MAXCHAR];
	int login_count;
	int update_count;
	char ldate[SOTADB_MAXCHAR];
};

extern MYSQL mysql;

int db_init(void);
int db_close(void);

int db_create_release_table_ifnotexist(char *tbl);
int db_create_ecu_table_ifnotexist(char *tbl);

int db_insert_row(char *tbl, struct sotatbl_row *row);
int db_search_col_str(char *tbl, char *column, char *value);
int db_search_col_int(char *tbl, char *column, int value);


#endif
