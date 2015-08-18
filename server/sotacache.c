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
#include "sotajson.h"
#include "sotacommon.h"

#define STRSIZE (64)
#define CMDSIZE (256)


extern struct download_info DownloadInfo;


int get_cache_dir(char *pathc, char *pathn, char *dirn)
{
	char cur[STRSIZE], new[STRSIZE];

	if((pathc == NULL) || (pathn == NULL) || (dirn == NULL))
		return -1;

	get_release_tag(pathc, cur);
	get_release_tag(pathn, new);

	sprintf(dirn, "%s/%s-%s", CachePath, cur, new);

	return 0;
}

int increment_cache_dir_usecount(char *path)
{
	FILE *fp;
	int cnt;
	char buf[STRSIZE] = "";

	if(path == NULL)
		return -1;

	if((fp = fopen(path, "rw")) == NULL) {
		printf("Can't open file: %s\n", path);
		return -1;
	}

	fgets(buf, STRSIZE, fp);
	cnt = atoi(buf);
	cnt++;
	fprintf(fp, "%d", cnt);

	fclose(fp);


	return 0;
}


/*************************************************************************
 * Function: extract_downloadinfo_fromcache
 *
 * This function extracts the download information from the json file in
 * cache directory to DownloadInfo structure.
 *
 * Returns:  0 - success
 *          -1 - error
 */
int extract_downloadinfo_fromcache(char *path)
{
	json_t *jp;
	int ret;
	char difile[JSON_NAME_SIZE];

	sprintf(difile, "%s/di.json", path);
	ret = sj_load_file(difile, &jp);
	if(ret < 0)
		return -1;

	sj_get_string(jp, "new_version", DownloadInfo.new_version);
	sj_get_string(jp, "sha256sum_diff", DownloadInfo.sh256_diff);
	sj_get_string(jp, "sha256sum_full", DownloadInfo.sh256_full);
	sj_get_int(jp, "original_size", &DownloadInfo.origsize);
	sj_get_int(jp, "compress_type", &DownloadInfo.compression_type);
	sj_get_int(jp, "compressed_diff_size", &DownloadInfo.compdiffsize);
	sj_get_int(jp, "file_parts", &DownloadInfo.fileparts);
	sj_get_int(jp, "lastpart_size", &DownloadInfo.lastpartsize);

	json_decref(jp);
	return 0;
}



/*************************************************************************
 * Function: store_downloadinfo_tocache
 *
 * This function stores the DownloadInfo structure as json file in cache
 * directory.
 *
 * Returns:  0 - success
 *          -1 - error
 */
int store_downloadinfo_tocache(char *path)
{
	json_t *jp;
	char difile[JSON_NAME_SIZE];

	jp = json_object();
	if(jp == NULL)
		return -1;

	sj_add_string(&jp, "new_version", DownloadInfo.new_version);
	sj_add_string(&jp, "sha256sum_diff", DownloadInfo.sh256_diff);
	sj_add_string(&jp, "sha256sum_full", DownloadInfo.sh256_full);
	sj_add_int(&jp, "original_size", DownloadInfo.origsize);
	sj_add_int(&jp, "compress_type", DownloadInfo.compression_type);
	sj_add_int(&jp, "compressed_diff_size", DownloadInfo.compdiffsize);
	sj_add_int(&jp, "file_parts", DownloadInfo.fileparts);
	sj_add_int(&jp, "lastpart_size", DownloadInfo.lastpartsize);

	sprintf(difile, "%s/di.json", path);
	if(0 > sj_store_file(jp, difile))
		return -1;

	json_decref(jp);
	return 0;
}



/*************************************************************************
 * Function: monitor_cache_dir
 *
 * This function monitors the disk usage of cache directory and checks if
 * the usage goes beyond the 'cache_size' parameter in server_info.json
 * file
 *
 * Return:
 *  -1: for errors
 *   0: if the usage is within the specified limit
 *   1: if the usage exceeded the limit
 */
int monitor_cache_dir(void)
{
	FILE *fp;
	char buf[CMDSIZE];
	char str[STRSIZE];
	int i, size;

	/* compute cache size */
	sprintf(buf, "du -sh %s > %s/cachesize.txt", CachePath, TempPath);
	system(buf);

	/* read the first line */
	sprintf(buf, "%s/cachesize.txt", TempPath);
	if((fp = fopen(buf, "rw")) == NULL) {
		printf("Can't open file: %s\n", buf);
		return -1;
	}
	fgets(str, STRSIZE, fp);
	fclose(fp);

	/* trim the line at first space char */
	for(i=0; i < STRSIZE; i++) {
		if(str[i] == ' ') {
			str[i] = '\0';
			break;
		}
	}
	size = atoi(str);
	if(size > CacheSize)
		return 1;

	return 0;
}


/*************************************************************************
 * Function: trim_cache_dir
 *
 * This function trims the disk usage of cache directory and brings the
 * the usage below the 'cache_size' parameter in server_info.json file
 *
 * This function assumes that there are no active connections when this is
 * called.
 *
 * Return:
 *	void
 */
void trim_cache_dir(void)
{
	printf("*** %s() under construction!! ***\n", __FUNCTION__);
}
