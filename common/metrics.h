/* 
 * Author: Aananth C N
 * email: c.n.aananth@gmail.com
 *
 * License: GPL v2
 * Date: 28 July 2015
 */

#ifndef METRICS_H 
#define METRICS_H



enum CAPTURE_TIMES {
	DOWNLOAD_TIME,
	UNCOMPRESSION_TIME,
	COMPRESSION_TIME,
	PATCH_TIME,
	TOTAL_TIME,
	MAX_CAPTURE_TIMES
};


int capture(int capt);
int print_metrics(void);



#endif
