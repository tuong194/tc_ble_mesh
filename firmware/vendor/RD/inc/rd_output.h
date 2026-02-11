/*
 * rd_output.h
 *
 *  Created on: Feb 11, 2026
 *      Author: PC5
 */

#ifndef RD_OUTPUT_H_
#define RD_OUTPUT_H_


#include "proj/common/types.h"
#include "rd_err.h"

#define OFF_STATE    0
#define ON_STATE     1

#define MAX_NUM_LED 2

#define LED_ONOFF  1
#define LED_SIGNAL 0

void led_init_gpio(void);
err_code_t led_set_state(uint8_t led_idx, uint8_t state);
uint8_t led_get_state(uint8_t led_idx);

void relay_init_gpio(void);
err_code_t relay_set_state(uint8_t state);
uint8_t relay_get_state(void);

#endif /* RD_OUTPUT_H_ */
