/* 
 * Author: Aananth C N
 * email: c.n.aananth@gmail.com
 *
 * License: MPL v2
 * Date: 20 Jan 2016
 */
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

#include "sotajson.h"
#include "sotaclient.h"
#include "sotacommon.h"
#include "metrics.h"
#include "sotamulti.h"
#include "unixcommon.h"



/*************************************************************************
 * Function: init_print_cb_ptr
 *
 * This function will be called for the usecases like Android JNI.
 * This function will save the callback function pointer in a global variable.
 */
void (*print_cb_fn)(char *str) = NULL;

void init_print_cb_ptr(void *cb_ptr)
{
	print_cb_fn = cb_ptr;
}

void print_callback(char *msg)
{
	if(print_cb_fn != NULL) {
		print_cb_fn(msg);
	}
}


void initialize_callbacks(char *cbstr)
{
	long addr;

	addr = atol(cbstr);
	if(addr != 0) {
		init_print_cb_ptr((void*) addr);
	}
	else {
		print("callback function called with null addr\n");
	}
}



