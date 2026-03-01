/*
 * btn_mgmt.h
 *
 *  Created on: Feb 10, 2026
 *      Author: PC5
 */

#ifndef BTN_MGMT_H_
#define BTN_MGMT_H_

#include "button.h"
#include "utils.h"

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



typedef enum {
    EVENT_BUTTON_PRESS = 1,  // press key
    EVENT_BUTTON_PAIR_K9B,   // pair CT2C 
    EVENT_BUTTON_DELETE_ALL_K9B, // delete all CT2C with one button
    EVENT_BUTTON_KICK_OUT,

    EVENT_BUTTON_MAX
} btn_event_id_t;

err_code_t btn_mgmt_gpio_config(void);
void btn_mgmt_register_event_handle(event_post_cb_t cb);


#endif /* BTN_MGMT_H_ */
