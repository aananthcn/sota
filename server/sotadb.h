/* 
 * Author: Aananth C N
 * email: caananth@visteon.com
 *
 * License: GPL v2
 * Date: 14 July 2015
 */
#ifndef SOTADB_H
#define SOTADB_H


#define SOTADB_SERVER	"localhost"
#define SOTADB_USERNM	"sota"
#define SOTADB_PASSWD	"visteon"
#define SOTADB_DBNAME	"sotadb"
#define SOTADB_MAXCHAR	255

#define SOTATBL_VEHICLE	"sotatbl"

struct client_tbl_row {
	int id;
	char vin[SOTADB_MAXCHAR];
	char serial_no[SOTADB_MAXCHAR];
	char name[SOTADB_MAXCHAR];
	char phone[SOTADB_MAXCHAR];
	char email[SOTADB_MAXCHAR];
	char make[SOTADB_MAXCHAR];
	char model[SOTADB_MAXCHAR];
	char device[SOTADB_MAXCHAR];
	char variant[SOTADB_MAXCHAR];
	int year;
};

int db_init(void);
int db_close(void);


int db_insert_row(char *tbl, struct client_tbl_row *row);
int db_search_col_str(char *tbl, char *column, char *value);
int db_search_col_int(char *tbl, char *column, int value);


#endif
