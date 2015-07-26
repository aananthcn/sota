/* 
 * Author: Aananth C N
 * email: c.n.aananth@gmail.com
 *
 * License: GPL v2
 * Date: 16 July 2015
 */
#ifndef SOTASERVER_H
#define SOTASERVER_H

extern unsigned long Sessions;

typedef enum SERVER_STATES {
	SS_INIT_STATE,
	SS_QUERY_STATE,
	SS_DWNLD_STATE,
	SS_FINAL_STATE,
	SS_CTRLD_STATE,
	MAX_SS_SERVER_STATES
}SERVER_STATES_T;

#endif
