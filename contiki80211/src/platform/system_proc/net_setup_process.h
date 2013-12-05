/*
 * net_setup_process.h
 *
 * Created: 5/14/2013 4:44:28 PM
 *  Author: Ioannis Glaropoulos
 */ 
#include "process.h"

#ifndef NET_SETUP_PROCESS_H_
#define NET_SETUP_PROCESS_H_

/*---------------------------------------------------------------------------*/
PROCESS_NAME(net_setup_process);


/* 
 * Indicates whether the network setup has been completed. 
 */
bool is_net_setup_completed();

/*---------------------------------------------------------------------------*/
#endif /* NET_SETUP_PROCESS_H_ */