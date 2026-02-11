/*
 * rd_output.c
 *
 *  Created on: Feb 11, 2026
 *      Author: PC5
 */

#include "drivers/8258/gpio.h"
#include "../inc/rd_output.h"
#include "../inc/utils.h"

#define ACTIVE_LEVEL_HIGH 1
#define ACTIVE_LEVEL_LOW  0

//typedef uint8_t active_level_t

typedef struct {
	GPIO_PinTypeDef gpio_pin;
    uint8_t active_level;
    s8 state;
}output_t;

static inline uint8_t real_level_set(output_t *out, uint8_t state){
	if(out->active_level == ACTIVE_LEVEL_HIGH){
		return state == ON_STATE ? 1:0;
	}
	return state == ON_STATE ? 0:1;
}

static inline void output_set_hw(output_t *out){
	uint8_t state = real_level_set(out, out->state);
	gpio_write(out->gpio_pin, (unsigned int)state);
}

err_code_t output_set_state(output_t *output, uint8_t state)
{
    if (output->state == state)
    {
//    	LOGE("err set state");
        return CODE_ERR;
    }
    output->state = state;
    output_set_hw(output);
    return CODE_OK;
}

void output_init_gpio(output_t *out){
	gpio_set_func(out->gpio_pin ,AS_GPIO);
	gpio_set_output_en(out->gpio_pin, 1);
	gpio_set_input_en(out->gpio_pin ,0);
}

/************************ LED, RELAY ****************************/

#define LED_SIGNAL_PIN GPIO_PD4
#define LED_ONOFF_PIN  GPIO_PC2

#define RELAY_INA_PIN  GPIO_PC0
#define RELAY_INB_PIN  GPIO_PB7

static output_t led[MAX_NUM_LED] = {
		{
			.gpio_pin = LED_SIGNAL_PIN,
			.active_level = ACTIVE_LEVEL_LOW,
			.state = -1,
		},
		{
			.gpio_pin = LED_ONOFF_PIN,
			.active_level = ACTIVE_LEVEL_LOW,
			.state = -1,
		}
};

static output_t relay[2] = {
		{
			.gpio_pin = RELAY_INA_PIN,
			.active_level = ACTIVE_LEVEL_HIGH,
			.state = -1,
		},
		{
			.gpio_pin = RELAY_INB_PIN,
			.active_level = ACTIVE_LEVEL_HIGH,
			.state = -1,
		}
};

void led_init_gpio(void){
	output_init_gpio(&led[0]);
	output_init_gpio(&led[1]);
	output_set_state(&led[0], ON_STATE);
	output_set_state(&led[1], ON_STATE);
}

err_code_t led_set_state(uint8_t led_idx, uint8_t state){
	if(led_idx > MAX_NUM_LED) return CODE_ERR;
	if(state != ON_STATE && state != OFF_STATE){
		LOGD("ERR: state value invalid: %02x", state);
		return ERR_INVALID_ARG;
	}
	return output_set_state(&led[led_idx], state);
}

uint8_t led_get_state(uint8_t led_idx){
	if(led_idx > MAX_NUM_LED) return CODE_ERR;
	return led[led_idx].state;
}

void relay_init_gpio(void){
	output_init_gpio(&relay[0]);
	output_init_gpio(&relay[1]);
	output_set_state(&relay[0], OFF_STATE);
	output_set_state(&relay[1], OFF_STATE);
}

err_code_t relay_set_state(uint8_t state){
	if(state != ON_STATE && state != OFF_STATE){
		LOGE("state value invalid: %02x", state);
		return ERR_INVALID_ARG;
	}
	if(state == ON_STATE){
		output_set_state(&relay[RELAY_INA], ON_STATE);
		output_set_state(&relay[RELAY_INB], OFF_STATE);
	}else{
		output_set_state(&relay[RELAY_INA], OFF_STATE);
		output_set_state(&relay[RELAY_INB], OFF_STATE);
	}
	return CODE_OK;
}
uint8_t relay_get_state(void){
	if(relay[RELAY_INA].state == relay[RELAY_INB].state)
		return OFF_STATE;
	return ON_STATE;
}




