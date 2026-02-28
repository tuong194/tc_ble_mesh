/*
 * controller.h
 *
 *  Created on: Feb 11, 2026
 *      Author: PC5
 */

#ifndef CONTROLLER_H_
#define CONTROLLER_H_

 //#include "btn_mgmt.h"

#define RD_SAFETY   1
#define RD_ERROR    0

void controller_init(void);
void task_bl0942(void);

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


#endif /* CONTROLLER_H_ */
