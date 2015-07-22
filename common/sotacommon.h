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
	char sw_version[JSON_NAME_SIZE];
};


#define SOTA_FILE_CHUNK_SIZE	(100*1024)

enum compression_types {
	SOTA_NO_COMP,
	SOTA_ZIP,
	SOTA_BZIP2,
	SOTA_XZ,
	MAX_SOTA_COMP_TYPE
};

struct download_info {
	char path[JSON_NAME_SIZE];
	char new_version[JSON_NAME_SIZE];
	int origsize;
	int compdiffsize;
	int compression_type;
	int filechunks;
	int lastchunksize;
	char sha256sum[JSON_NAME_SIZE];
};


extern char SessionPath[JSON_NAME_SIZE];

#endif
