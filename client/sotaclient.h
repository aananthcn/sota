/* 
 * Author: Aananth C N
 * email: c.n.aananth@gmail.com
 *
 * License: GPL v2
 * Date: 16 July 2015
 */
#ifndef SOTACLIENT_H
#define SOTACLIENT_H

typedef enum CLIENT_STATES {
	SC_REGTN_STATE,
	SC_LOGIN_STATE,
	SC_QUERY_STATE,
	SC_DWNLD_STATE,
	SC_FINAL_STATE,
	SC_CTRLD_STATE,
	MAX_SC_CLIENT_STATES
}CLIENT_STATES_T;


#endif
