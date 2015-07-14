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
#define SOTADB_CREATE	"CREATE DATABASE sotadb"


int init_sotadb(void);
int close_sotadb(void);



#endif
