/* 
 * Author: Aananth C N
 * email: c.n.aananth@gmail.com
 *
 * License: GPL v2
 * Date: 16 August 2015
 */
#include <stdio.h>
#include <string.h>

#include "sotaserver.h"
#include "swreleases.h"


int get_cache_dir(char *pathc, char *pathn, char *dirn)
{
	char cur[64], new[64];

	if((pathc == NULL) || (pathn == NULL) || (dirn == NULL))
		return -1;

	get_release_tag(pathc, cur);
	get_release_tag(pathn, new);

	sprintf(dirn, "%s/%s-%s", CachePath, cur, new);

	return 0;
}
