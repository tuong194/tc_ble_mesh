/*
 * mess_handle.h
 *
 *  Created on: Feb 12, 2026
 *      Author: PC5
 */

#ifndef MESS_HANDLE_H_
#define MESS_HANDLE_H_

#include "proj/common/types.h"
#include "proj_lib/sig_mesh/app_mesh.h"

#define RD_OPCODE_SCAN_DEV            0xE0
#define RD_OPCODE_RSP_SCAN_DEV        0xE1
#define RD_OPCODE_PRODUCT_FEATURE     0xE2
#define RD_OPCODE_RSP_PRODUCT_FEATURE 0xE3

#define RD_HEADER_GET_TYPE          0x0001
#define RD_HEADER_SAVE_GATEWAY      0x0002
#define RD_HEADER_AES_AND_GET_TYPE  0x0003

#define RD_HEADER_GET_VOLTAGE
#define RD_HEADER_GET_CURRENT
#define RD_HEADER_GET_POWER
#define RD_HEADER_SET_THRESHOLD

int RD_mess_handle_opcode_E0(u8 *par, int par_len, mesh_cb_fun_par_t * cb_par);
int RD_mess_handle_opcode_E2(u8 *par, int par_len, mesh_cb_fun_par_t * cb_par);

#endif /* MESS_HANDLE_H_ */
