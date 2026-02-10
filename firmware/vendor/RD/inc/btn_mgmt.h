/*
 * btn_mgmt.h
 *
 *  Created on: Feb 10, 2026
 *      Author: PC5
 */

#ifndef BTN_MGMT_H_
#define BTN_MGMT_H_

#include "button.h"

#define MAX_NUM_BUTTON 1

#define CONFIG_PRESS_TIME_MS     100
#define CONFIG_KEEP_TIME_MS      3000
#define CONFIG_LONG_KEEP_TIME_MS 6000

#define CLOCK_TIME_SET_PAIR_K9B  (3*1000*1000 - CONFIG_KEEP_TIME_MS*1000)
#define CLOCK_TIME_KICK_OUT      (3*1000*1000 - CONFIG_KEEP_TIME_MS*1000)
#define CLOCK_TIME_OUT_KICK_OUT  (10*1000*1000 - CONFIG_KEEP_TIME_MS*1000)

#define ACTIVE_HIGH 1
#define ACTIVE_LOW  0

#define BUTTON_ACTIVE_LEVEL ACTIVE_LOW
#define BUTTON_GPIO_PIN     GPIO_PA1

err_code_t button_gpio_config(void);


#endif /* BTN_MGMT_H_ */
