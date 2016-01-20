#ifndef SOTA_CALLBACKS_H
#define SOTA_CALLBACKS_H
/* 
 * Author: Aananth C N
 * email: c.n.aananth@gmail.com
 *
 * License: MPL v2
 * Date: 20 Jan 2016
 */


/* Init function */
void init_print_cb_ptr(void *cb_ptr);


/* 
 * SOTA client supports following callbacks for android or other clients 
 */
void print_callback(char *msg);










#endif
