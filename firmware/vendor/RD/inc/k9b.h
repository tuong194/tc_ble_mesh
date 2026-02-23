/*
 * k9b.h
 *
 *  Created on: Feb 12, 2026
 *      Author: PC5
 */

#ifndef K9B_H_
#define K9B_H_

#include "rd_err.h"

#define TIME_OUT_SCAN_KP9       10*1000*1000
#define TIME_OUT_PRESS          500*1000
#define RD_GW_ADDR              0x0001

#define MAX_NUM_K9ONOFF         5
#define MAX_NUM_K9B_HC          5  //hc
#define MAX_NUM_K9B_PRESS_STYLE 12


typedef struct
{
	uint8_t length; // payload length
	uint8_t type_adv; // type 
	uint8_t vid[2]; // vender ID
	uint8_t frame; //frame
	uint8_t counter[4]; // number of press
	uint8_t type_device; // device type 
	uint8_t key; 
	uint32_t signature; 
} k9b_proxy_t;


#endif /* K9B_H_ */
