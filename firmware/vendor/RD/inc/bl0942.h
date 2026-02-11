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

#define  BL0942_REG_NONE 0xFF

// Rt (350.0f)
//#define MULTIPLIER_U  (2375.72118f/(73989.0f*510.0f))  //((1.218f * (1200000.0f+750000.0f+510.0f))/(73989.0f*510.0f*1000.0f))
//#define MULTIPLIER_I  2.0f*(0.8526f/305978.0f) //(1.218f/(305978.0f * 0.5f * 1000.0f/350.0f))
//#define MULTIPLIER_P  2.3f*(2025.539878f/(4096.0f*510.0f)) //((1.218f * 1.218f * (1200.0f+750.0f+0.51f))/(4096.0f * (0.5f * 1000.0f/350.0f) * 0.51f * 1000.0f))

#define MULTIPLIER_U  (2375.72118f/(73989.0f*510.0f))  //((1.218f * (1200000.0f+750000.0f+510.0f))/(73989.0f*510.0f*1000.0f))
#define MULTIPLIER_I  (1.7052f/305978.0f) //(1.218f/(305978.0f * 0.5f * 1000.0f/700.0f))
#define MULTIPLIER_P  (4051.079756f/(3537.0f*510.0f)) //((1.218f * 1.218f * (1200.0f+750.0f+0.51f))/(3537.0f * (0.5f * 1000.0f/700.0f) * 0.51f * 1000.0f))


err_code_t bl0942_init(void);
uint32_t bl0942_read_data_unsigned(uint8_t REG);
s32 bl0942_read_data_signed(uint8_t REG);

#endif /* BL0942_H_ */
