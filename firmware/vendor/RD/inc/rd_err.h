/*
 * rd_err.h
 *
 *  Created on: Feb 9, 2026
 *      Author: PC5
 */

#ifndef RD_ERR_H_
#define RD_ERR_H_

#include "proj/common/types.h"

#define err_code_t s16

#define CODE_ERR -1
#define CODE_OK   0
#define VOLTAGE_LOW_BACK_TO_NORMAL 9
#define VOLTAGE_HIGH_BACK_TO_NORMAL 10
#define CURRENT_LOW_BACK_TO_NORMAL 11
#define CURRENT_HIGH_BACK_TO_NORMAL 12
#define POWER_ERR_BACK_TO_NORMAL 13

#define ERR_INVALID_ARG 0x101
#define ERR_CONFIG_GPIO 0x102
#define ERR_SET_VALUE 0x103
#define ERR_DETECT_ELECTRICAL 0x104
#define ERR_VOL_TOO_HIGH 0x105
#define ERR_VOL_TOO_LOW 0x106
#define ERR_CUR_TOO_HIGH 0x017
#define ERR_CUR_TOO_LOW 0x108
#define ERR_POWER 0x109


#endif /* RD_ERR_H_ */
