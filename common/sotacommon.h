/* 
 * Author: Aananth C N
 * email: c.n.aananth@gmail.com
 *
 * License: GPL v2
 * Date: 19 July 2015
 */
#ifndef SOTACOMMON_H
#define SOTACOMMON_H

#include "sotajson.h"

struct client {
	int id;
	char vin[JSON_NAME_SIZE];
	char name[JSON_NAME_SIZE];
	char sw_version[JSON_NAME_SIZE];
	char sw_path[JSON_NAME_SIZE];
};


#define SOTA_FILE_PART_SIZE	(100*1024)

enum compression_types {
	SOTA_NO_COMP,
	SOTA_ZIP,
	SOTA_BZIP2,
	SOTA_XZ,
	MAX_SOTA_COMP_TYPE
};

struct download_info {
	char new_version[JSON_NAME_SIZE];
	int origsize;
	int compdiffsize;
	char compdiffpath[JSON_NAME_SIZE];
	int compression_type;
	int fileparts;
	int lastpartsize;
	char sh256_diff[JSON_NAME_SIZE];
	char sh256_full[JSON_NAME_SIZE];
};


typedef char strname_t[JSON_NAME_SIZE];

extern char SessionPath[JSON_NAME_SIZE];
extern struct download_info DownloadInfo;
extern int Debug;

#endif
