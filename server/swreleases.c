/* 
 * Author: Aananth C N
 * email: c.n.aananth@gmail.com
 *
 * License: GPL v2
 * Date: 14 August 2015
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sotadb.h"
#include "sotajson.h"
#include "sotaserver.h"

#define QRYSIZE (128*1024)
#define STRSIZE (256)
#define PATHLEN (1024)



int checktag_update_reltbl(char *tbl, char *path, char *tag)
{
	int ret;
	char query[QRYSIZE];
	char dbpath[PATHLEN];

	/* check if this tag exist in database */
	if(db_check_col_str(tbl, "sw_version", tag) > 0) {
		ret = db_get_columnstr_fromkeystr(tbl, "path",
						  dbpath, "sw_version", tag);
		if(ret < 0)
			return -1;

		if(0 == strcmp(dbpath, path)) {
			/* set active flag if both tag and path are matching */
			db_set_columnint_fromkeystr(tbl, "active",
						    1, "sw_version", tag);
			return 0;
		}
		else {
			/* delete row if path don't match */
			sprintf(query, "DELETE FROM %s.%s WHERE sw_version = \'%s\'",
			       SOTADB_DBNAME, tbl, tag);
			if(0 != mysql_query(&mysql, query)) {
				db_print_error(query);
				return -1;
			}
		}
	}

	/* got a new tag, insert this to release table */
	sprintf(query, "INSERT INTO %s.%s (sw_version, path, active) VALUES (\'%s\', \'%s\', \'%d\')",
		SOTADB_DBNAME, tbl, tag, path, 1);
	if(0 != mysql_query(&mysql, query)) {
		db_print_error(query);
		return -1;
	}

	return 0;
}


int get_release_tag(char *line, char *tag)
{
	int len, i, start, end;
	int state = 0;

	if((line == NULL) || (tag == NULL))
		return -1;

calculate_len:
	len = strlen(line);
	if(len > PATHLEN)
		return -1;

	if(line[len-1] == '\n') {
		line[len-1] = '\0';
		goto calculate_len; /* again */
	}

	for(i=len; i >= 0; i--) {
		switch (state) {
		case 0:
			if(line[i] == '/') {
				end = i;
				state = 1; /* end of tag */
			}
			break;
		case 1:
			if(line[i] == '/') {
				start = i+1;
				state = 2; /* start of tag */
			}
			break;
		default:
			break;
		}

		/* check if we can get the release tag */
		if(state >= 2) {
			strncpy(tag, line+start,(end-start));
			tag[end-start] = '\0';
			break;
		}
	}

	return 0;
}



int invalidate_all_releases(char *tbl)
{
	char query[QRYSIZE];

	if(tbl == NULL) {
		printf("%s(), invalid input\n", __FUNCTION__);
		return -1;
	}

	sprintf(query, "update %s.%s set active=0", SOTADB_DBNAME, tbl);
	if(0 != mysql_query(&mysql, query)) {
		db_print_error(query);
		return -1;
	}

	return 0;
}




int check_lower_case_rule(char *str)
{
	int len, i;

	len = strlen(str);
	for(i = 0; i < len; i++) {
		if(isupper(str[i])) {
			printf("Error: Case violation in directory names\n");
			printf("Change all directory and sub-directory of ");
			printf("\"%s\" to lower case!!\n", ReleasePath);
			return -1;
		}
	}

	return 0;
}


/*************************************************************************
 * Function: get_reltbl_name
 *
 * This function is called when the client requests for new software
 * updates. This is much later to parse_devices function below.
 *
 * This function copies the release table name from the vin and ecu name
 * to the last argument.
 *
 * return: -1 in case of error
 */
int get_reltbl_name(char *vin, char *ecu_name, char *tbl)
{
	char make[JSON_NAME_SIZE];
	char model[JSON_NAME_SIZE];
	int ret;

	if((tbl == NULL) || (vin == NULL) || (ecu_name == NULL)) {
		printf("%s(), invalid args\n", __FUNCTION__);
		return -1;
	}

	if(0 > db_get_make_model(SOTATBL_VEHICLE, vin, make, model))
		return -1;

	/* following line and parse_devices fn should match */
	sprintf(tbl, "%s_%s_%s_reltbl", make, model, ecu_name);

	return 0;
}


/**************************************************************************
 * Function: parse_releases
 *
 * This function parses the release file lists corresponding to a device 
 * (i.e., cluster, headunit etc) and extract release tag and populate the
 * mysql table if the release tag is new.
 *
 * Note: the release table is passed as 2nd argument and the name of the 
 * table is as detailed in comments section of update_swrelease_and_tables
 * function
 */
int parse_releases(char *releaselist, char *tbl)
{
	int i, releases;
	char path[PATHLEN];
	char tag[PATHLEN];
	FILE *fp;


	if((fp = fopen(releaselist, "r")) == NULL) {
		printf("Can't open file: %s\n", releaselist);
		return -1;
	}

	/* create release table if not exist */
	if(0 > check_lower_case_rule(tbl))
		return -1;
	db_create_reltbl_ifnotexist(tbl);
	invalidate_all_releases(tbl);

	/* update release table with releases in list */
	releases = get_filelines(releaselist);
	for(i=0; (i < releases) && (fgets(path, PATHLEN, fp)); i++) {
		if(0 > get_release_tag(path, tag))
			continue;
		checktag_update_reltbl(tbl, path, tag); /* fixme: large db takes more time */
	}

	fclose(fp);
}


/**************************************************************************
 * Function: parse_ecus
 *
 * This function parses the ecus list (i.e., directories such as cluster,
 * headunit under make_model directories) and prepare a release list.
 * 
 * It is expected that, the ecu manufacturers will place their new sw
 * releases under these directories with release tag names.
 */
int parse_ecus(char *veh, char *eculist)
{
	int i, ecus, len;
	int ret = 0;
	char ecu[PATHLEN];
	char relpath[PATHLEN];
	char cmd[PATHLEN];
	char listfile[PATHLEN];
	char reltbl[PATHLEN];
	FILE *fp;


	if((fp = fopen(eculist, "r")) == NULL) {
		printf("Can't open file: %s\n", eculist);
		return -1;
	}

	ecus = get_filelines(eculist);
	for(i=0; (i < ecus) && (fgets(ecu, PATHLEN, fp)); i++) {
		len = strlen(ecu);
		if(ecu[len-1] == '\n')
			ecu[len-1] = '\0';
		/* get release list */
		sprintf(relpath, "%s/%s/%s", ReleasePath, veh, ecu);
		sprintf(listfile, "%s/releases.txt", SessionPath);
		printf("\t");
		sprintf(cmd, "find %s -name %s > %s", relpath, RelFileName,
			listfile);
		system(cmd);

		/* following line and get_reltbl_name fn should match */
		sprintf(reltbl, "%s_%s_reltbl", veh, ecu);
		if(0 > parse_releases(listfile, reltbl))
			ret = -1;
	}

	fclose(fp);
	return ret;
}



/**************************************************************************
 * Function: parse_vehicles
 *
 * This function scans and create a list of sub-directories under vehicles
 * (i.e., make_model) directories under the main software release path in
 * sota server.
 */
int parse_vehicles(char *vehlist)
{
	int i, vehicles, len;
	int ret = 0;
	char vehicle[PATHLEN];
	char ecupath[PATHLEN];
	char cmd[PATHLEN];
	char listfile[PATHLEN];
	FILE *fp;


	if((fp = fopen(vehlist, "r")) == NULL) {
		printf("Can't open file: %s\n", vehlist);
		return -1;
	}

	/* parse through make_model and ecu directories */
	vehicles = get_filelines(vehlist);
	for(i=0; (i < vehicles) && (fgets(vehicle, PATHLEN, fp)); i++) {
		/* eliminate white space */
		len = strlen(vehicle);
		if(vehicle[len-1] == '\n')
			vehicle[len-1] = '\0';
		/* get ecu list */
		sprintf(ecupath, "%s/%s", ReleasePath, vehicle);
		sprintf(listfile, "%s/ecus.txt", SessionPath);
		printf("\t");
		sprintf(cmd, "ls %s > %s", ecupath, listfile);
		system(cmd);
		if(0 > parse_ecus(vehicle, listfile))
			ret = -1;
	}

	fclose(fp);
	return ret;
}


/*************************************************************************
 * Function: update_swrelease_and_tables
 *
 * This function creates and updates software release tables in MYSQL
 * database. The creation of table is based on assumption that software
 * releases from different projects are stored as tree structure shown
 * below
 *
 * swrelease_path (check server_info.json file)
 *    |
 *    +--vehicle (or make_model e.g., mahindra_w207)
 *    |    |
 *    |    +--ecu (or ecu_name e.g., cluster, headunit)
 *    |         |
 *    |         +--release_tag-1
 *    |         +--release_tag-2
 *    .         ..
 *
 *
 *  The sw release table names in MYSQL will be named as below:
 *  Eg. mahindra_w207_headunit_reltbl, ford_d544_cluster_reltbl
 */
int update_swreleases_and_tables(void)
{
	char listfile[JSON_NAME_SIZE];
	char cmd[PATHLEN];

	printf("   checking for new releases...\n\t");

	/* get make_model list */
	sprintf(listfile, "%s/vehicles.txt", SessionPath);
	sprintf(cmd, "ls %s > %s", ReleasePath, listfile);
	system(cmd);
	if(0 > parse_vehicles(listfile))
		return -1;

	return 0;
}
