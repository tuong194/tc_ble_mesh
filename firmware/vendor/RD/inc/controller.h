/*
 * controller.h
 *
 *  Created on: Feb 11, 2026
 *      Author: PC5
 */

#ifndef CONTROLLER_H_
#define CONTROLLER_H_

 //#include "btn_mgmt.h"
#define I_THRESHOLD_DEFAULT   100   // /100 A
#define P_THRESHOLD_DEFAULT   50000  // /100 W
#define TIME_CYCLE_READ_MS_DF    10000
#define MAX_CYCLE_DETECT_ERROR_I_DF 5
#define MAX_CYCLE_DETECT_ERROR_P_DF 5

#define RD_SAFETY   1
#define RD_ERROR    0

typedef enum{
    EVENT_SECURE_BIND_ALL = 0,
    EVENT_SECURE_ENCRYPT_DONE,
    EVENT_SECURE_ENCRYPT_FAIL,
    EVENT_SECURE_MAX
}secure_event_t;

void controller_init(void);
void task_bl0942(void);
void task_check_kick_out(void);

void    dev_set_state(uint8_t onoff);
uint8_t dev_get_state(void);

uint32_t aptomat_get_voltage(void);
uint32_t aptomat_get_current(void);
uint32_t aptomat_get_power(void);
uint32_t aptomat_get_power_consume(void);

void 	 dev_set_threshold_power(uint32_t thres_val);
void 	 dev_set_threshold_current(uint32_t thres_val);
void     dev_set_time_cycle_read_param_electrical(uint32_t time_ms);
void 	 dev_set_max_num_detect_err_current(uint8_t num);
void 	 dev_set_max_num_detect_err_power(uint8_t num);

void rd_write_flash_common(void);
void rd_dev_clear_secure(void);


#endif /* CONTROLLER_H_ */
