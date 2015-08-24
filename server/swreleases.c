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


static int Releases = -1;


int checktag_update_reltbl(char *tag, char *path)
{
	int ret;
	char query[QRYSIZE];
	char dbpath[PATHLEN];

	/* check if this tag exist in database */
	if(db_check_col_str(SwReleaseTbl, "sw_version", tag) > 0) {
		ret = db_get_columnstr_fromkeystr(SwReleaseTbl, "path",
						  dbpath, "sw_version", tag);
		if(ret < 0)
			return -1;

		if(0 == strcmp(dbpath, path)) {
			/* set active flag if both tag and path are matching */
			db_set_columnint_fromkeystr(SwReleaseTbl, "active",
						    1, "sw_version", tag);
			return 0;
		}
		else {
			/* delete row if path don't match */
			sprintf(query, "DELETE FROM %s.%s WHERE sw_version = \'%s\'",
			       SOTADB_DBNAME, SwReleaseTbl, tag);
			if(0 != mysql_query(&mysql, query)) {
				db_print_error(query);
				return -1;
			}
		}
	}

	/* got a new tag, insert this to release table */
	sprintf(query, "INSERT INTO %s.%s (sw_version, path, active) VALUES (\'%s\', \'%s\', \'%d\')",
		SOTADB_DBNAME, SwReleaseTbl, tag, path, 1);
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




int create_table_ifnotexist(char *tbl)
{
	char query[QRYSIZE];
	MYSQL_RES *res;
	int rows;

	sprintf(query, "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema = \'%s\' AND table_name = \'%s\'", SOTADB_DBNAME, tbl);
	if(0 != mysql_query(&mysql, query)) {
		db_print_error(query);
		return -1;
	}

	res = mysql_use_result(&mysql);
	if(res == NULL) {
		db_print_error("use result call");
		return -1;
	}
	rows = mysql_num_rows(res);
	mysql_free_result(res);

	/* check if table exists */
	if(rows == 0) {
		sprintf(query, "CREATE TABLE IF NOT EXISTS %s.%s (sw_version varchar(255), path varchar(255), active int, PRIMARY KEY (sw_version))", SOTADB_DBNAME, tbl);
		if(0 != mysql_query(&mysql, query)) {
			db_print_error(query);
			return -1;
		}
	}

	return 0;
}



int get_make_model(char *vin, char *make, char *model)
{
	int ret, i;

	if((vin == NULL) || (make == NULL) || (model == NULL)) {
		printf("%s(), invalid inputs\n", __FUNCTION__);
		return -1;
	}

	ret = db_get_columnstr_fromkeystr(SOTATBL_VEHICLE, "make", make,
					  "vin", vin);
	if(ret < 0) {
		printf("%s(), can't get make from vin\n", __FUNCTION__);
		return -1;
	}

	ret = db_get_columnstr_fromkeystr(SOTATBL_VEHICLE, "model", model,
					  "vin", vin);
	if(ret < 0) {
		printf("%s(), can't get model from vin\n", __FUNCTION__);
		return -1;
	}

	/* convert to lower case to avoid path issues */
	for(i=0; make[i] && (i < STRSIZE); i++)
		make[i] = tolower(make[i]);
	for(i=0; model[i] && (i < STRSIZE); i++)
		model[i] = tolower(model[i]);

	return 0;
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
 *    +--make
 *    |    |
 *    |    +--model
 *    |         |
 *    |         +--release_tag-1
 *    |         +--release_tag-2
 *    .         ..
 *
 *
 *  The sw release table names in MYSQL will be named as make-model-tbl.
 *  Eg. mahindra-w207-tbl, ford-d544-tbl
 */
int update_swreleases_and_tables(char *vin)
{
	int releases, i;
	FILE *fp;
	char relnames[JSON_NAME_SIZE];
	char buf[PATHLEN];
	char tag[PATHLEN];
	char make[STRSIZE];
	char model[STRSIZE];

	/* get the right table path and name */
	if(0 > get_make_model(vin, make, model))
		return -1;
	sprintf(buf, "/%s/%s/", make, model);
	strcat(ReleasePath, buf);
	sprintf(SwReleaseTbl, "%s_%s_tbl", make, model);
	create_table_ifnotexist(SwReleaseTbl);

	/* count the releases */
	printf("   checking for new releases...\n\t");
	sprintf(relnames, "%s/releases.txt", SessionPath);
	sprintf(buf, "find %s -name %s > %s", ReleasePath, RelFileName,
		relnames);
	system(buf);
	releases = get_filelines(relnames);

	/* check if a new release happened by this time */
	if(releases == Releases) {
		printf("No new releases! Total - %d\n", releases);
		return 0;
	}

	/* if yes, then invalidate all releases */
	invalidate_all_releases(SwReleaseTbl);

	/* process tags and update database */
	if((fp = fopen(relnames, "r")) == NULL) {
		printf("Can't open file: %s\n", relnames);
		return -1;
	}
	for(i=0; (i < releases) && (fgets(buf, PATHLEN, fp)); i++) {
		if(0 > get_release_tag(buf, tag))
			continue;
		checktag_update_reltbl(tag, buf); /* fixme: large db takes more time */
	}
	fclose(fp);

	/* fixme: need to add semaphore before modifying globals */
	Releases = releases;

	return 0;
}
