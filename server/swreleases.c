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


static int Releases;

int checktag_update_reltbl(char *tag, char *path)
{
	char query[QRYSIZE];

	/* check if this tag exist in database */
	if(db_check_col_str(SOTATBL_SWRELES, "sw_version", tag) > 0) {
		db_set_columnint_fromkeystr(SOTATBL_SWRELES, "active", 1,
					    "sw_version", tag);
	}
	else {
		/* add this tag */
		sprintf(query, "INSERT INTO %s.%s (sw_version, path, active) VALUES (\'%s\', \'%s\', \'%d\')", SOTADB_DBNAME, SOTATBL_SWRELES, tag, path, 1);
		if(0 != mysql_query(&mysql, query)) {
			db_print_error(query);
			return -1;
		}
	}

	return 0;
}


char* get_release_tag(char *line)
{
	int len, i, start, end;
	int state = 0;
	char *tag = NULL;
	char buf[STRSIZE];

	if(line == NULL)
		return NULL;

	len = strlen(line);
	if(len > PATHLEN)
		return NULL;

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
			strncpy(buf, line+start,(end-start));
			tag = buf;
			break;
		}
	}

	return tag;
}



int invalidate_all_releases(char *tbl)
{
	char query[QRYSIZE];

	sprintf(query, "update %s.%s set active=0", SOTADB_DBNAME, tbl);
	if(0 != mysql_query(&mysql, query)) {
		db_print_error(query);
		return -1;
	}

	return 0;
}



int update_swreleases(void)
{
	int releases, i;
	FILE *fp;
	char *tag;
	char relnames[JSON_NAME_SIZE];
	char buf[PATHLEN];

	/* count the releases */
	sprintf(relnames, "%s/releases.txt", SessionPath);
	sprintf(buf, "find %s -name %s > %s", ReleasePath, RelFileName,
		relnames);
	system(buf);
	releases = get_filelines(relnames);

	/* check if a new release happened by this time */
	if(releases == Releases)
		return 0;

	/* if yes, then invalidate all releases */
	invalidate_all_releases(SOTATBL_SWRELES);

	/* process tags and update database */
	if((fp = fopen(relnames, "r")) == NULL) {
		printf("Can't open file: %s\n", relnames);
		return -1;
	}
	for(i=0; (i < releases) && (fgets(buf, PATHLEN, fp)); i++) {
		tag = get_release_tag(buf);
		if(tag == NULL)
			continue;
		checktag_update_reltbl(tag, buf); /* fixme: large db takes more time */
	}
	fclose(fp);

	/* fixme: need to add semaphore before modifying globals */
	Releases = releases;

	return 0;
}
