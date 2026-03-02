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
#include "utils.h"

#define RD_OPCODE_SCAN_DEV            0xE0
#define RD_OPCODE_RSP_SCAN_DEV        0xE1
#define RD_OPCODE_PRODUCT_FEATURE     0xE2
#define RD_OPCODE_RSP_PRODUCT_FEATURE 0xE3

#define RD_OPCODE_REPORT_PARAM_ELECTRICAL 0x52

#define RD_HEADER_GET_TYPE          0x0001
#define RD_HEADER_SAVE_GATEWAY      0x0002
#define RD_HEADER_AES_AND_GET_TYPE  0x0003

#define RD_HEADER_GET_PARAM               0xE405
#define RD_HEADER_SET_THRESHOLD_CURRENT   0x0417
#define RD_HEADER_SET_THRESHOLD_POWER     0x0517
#define RD_HEADER_COUNTDOWN               0x070B
#define RD_HEADER_SET_TIME_AND_NUM_DETECT 0xE505  // set time cycle read param electrical and set max num detect error

typedef enum {
    TYPE_GET_VOLTAGE = 0x00,
    TYPE_GET_CURRENT = 0x01,
    TYPE_GET_POWER = 0x02,
    TYPE_GET_POWER_CONSUME = 0x03
}type_get_para;



int RD_mess_handle_opcode_E0(u8* par, int par_len, mesh_cb_fun_par_t* cb_par);
int RD_mess_handle_opcode_E2(u8* par, int par_len, mesh_cb_fun_par_t* cb_par);

int dev_rsp_param_to_gw(type_get_para type);
void rd_register_event_secure(event_post_cb_t cb);
uint16_t rd_get_gateway_addr(void);
void init_flash_gateway(void);

#endif /* MESS_HANDLE_H_ */
