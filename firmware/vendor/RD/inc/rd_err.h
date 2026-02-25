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

#define ERR_INVALID_ARG 0x101
#define ERR_CONFIG_GPIO 0x102
#define ERR_SET_VALUE 0x103
#define ERR_DETECT_ELECTRICAL 0x104


#endif /* RD_ERR_H_ */
