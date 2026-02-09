/*
 * bl0942.h
 *
 *  Created on: Feb 9, 2026
 *      Author: PC5
 */

#ifndef BL0942_H_
#define BL0942_H_


#include "../inc/rd_err.h"


#define BL0942_CMD_WRITE 0xA8
#define BL0942_CMD_READ 0x58

#define BL0942_REG_UNLOCK 0x1D
#define BL0942_REG_SOFT_RESET 0x1C
#define BL0942_REG_GAIN_CR 0x1A
#define BL0942_REG_OT_FUNX 0x18
#define BL0942_REG_WATT 0x06
#define BL0942_REG_IRMS 0x03
#define BL0942_REG_VRMS 0x04

#define BL0942_REG_NONE 0xFF

err_code_t bl0942_init(void);
uint32_t bl0942_read_data_unsigned(uint8_t REG);
s32 bl0942_read_data_signed(uint8_t REG);

#endif /* BL0942_H_ */
