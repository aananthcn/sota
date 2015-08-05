/* 
 * Author: Aananth C N
 * email: c.n.aananth@gmail.com
 *
 * License: GPL v2
 * Date: 28 July 2015
 */
#include <stdio.h>
#include <sys/time.h>

#include "sotabdata.h"
#include "metrics.h"
#include "sotacommon.h"


struct time_stamp {
	unsigned long t1;
	unsigned long t2;
	unsigned long cnt;
	char name[256];
};

struct time_stamp TimeStamps[MAX_CAPTURE_TIMES] = {
	0, 0, 0, "Downloads",
	0, 0, 0, "Decompressions",
	0, 0, 0, "Compressions",
	0, 0, 0, "Patching",
	0, 0, 0, "Total (this includes server and communication delays)"
};


int capture(int capt)
{
	struct timeval time;

	if(capt >= MAX_CAPTURE_TIMES)
		return -1;

	gettimeofday(&time, NULL);
	TimeStamps[capt].t2 = TimeStamps[capt].t1;
	TimeStamps[capt].t1 = time.tv_sec;
	TimeStamps[capt].cnt++;

	return 0;
}

int print_metrics(void)
{
	int i, parts;
	int time;
	float ratio;
	float savings;

	if(DownloadInfo.fileparts == 0)
		return -1;

	printf("\n\nMETRICS:\n=======\n");

	for(i=0; i< MAX_CAPTURE_TIMES; i++) {
		/* check for no capture and unbalanced capture */
		if((TimeStamps[i].cnt) && ((TimeStamps[i].cnt & 1) == 0)) {
			time = TimeStamps[i].t1 - TimeStamps[i].t2;
			printf("%d mins, %d secs spent in \"%s\"\n",
			       time/60, time%60, TimeStamps[i].name);
		}
	}

	printf("\nMore Metrics:\n------------\n");
	printf("Size of release tar ball - %.3f MiB\n",
	       ((float)DownloadInfo.origsize)/(1024*1024));
	printf("Size of comp. diff file  - %.3f MiB\n",
	       ((float)DownloadInfo.compdiffsize)/(1024*1024));
	parts = DownloadInfo.fileparts + (DownloadInfo.lastpartsize ? 1 : 0);
	printf("Diff file chunks recv'd  - %d\n", parts);
	ratio = ((float)DownloadInfo.origsize) / DownloadInfo.compdiffsize;
	printf("Ratio of Diff : Orig tar - 1 : %.2f\n", ratio);
	printf("Savings in n/w bandwidth - %.2f\n", (100.0 - 100.0/ratio));
}
