/*
 * controller.h
 *
 *  Created on: Feb 11, 2026
 *      Author: PC5
 */

#ifndef CONTROLLER_H_
#define CONTROLLER_H_

 //#include "btn_mgmt.h"

#define TIME_CYCLE_READ_MS      5000

void controller_init(void);
void task_bl0942_read(void);

uint32_t aptomat_get_voltage(void);
uint32_t aptomat_get_current(void);
uint32_t aptomat_get_power(void);
uint32_t aptomat_get_power_consume(void);
void dev_set_threshold_power(uint32_t thres_val);
void dev_set_threshold_current(uint32_t thres_val);

#endif /* CONTROLLER_H_ */
