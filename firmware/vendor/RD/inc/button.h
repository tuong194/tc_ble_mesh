/*
 * button.h
 *
 *  Created on: Feb 10, 2026
 *      Author: PC5
 */

#ifndef BUTTON_H_
#define BUTTON_H_


#include "drivers/8258/gpio.h"
#include "rd_err.h"

typedef void *button_handle_t;
typedef void (* button_cb_t)(void *button_handle, void *usr_data);

typedef struct {
	GPIO_PinTypeDef gpio_num;       /**< num of gpio */
    uint8_t active_level;          /**< gpio level when press down */
    bool disable_pull;            /**< disable internal pull or not */
} button_gpio_config_t;

typedef struct{
    uint16_t press_time;
    uint16_t keep_time;
    uint16_t long_keep_time;
    button_gpio_config_t button_gpio_config;
}button_config_t;

typedef enum{
    BUTTON_EVENT_PRESS,                 // Button press
    BUTTON_EVENT_LONG_PRESS,               // Button is being held
    BUTTON_EVENT_RELEASE_LONG_PRESS,       // Release after holding
    BUTTON_EVENT_LONG_LONG_PRESS,          // Long press
    BUTTON_EVENT_RELEASE_LONG_LONG_PRESS,  // Release after long press
    BUTTON_EVENT_MAX,                   // Maximum number of events
    BUTTON_EVENT_NONE_PRESS             // No button press
}button_event_t;


void button_reset_touch_pin_config(GPIO_PinTypeDef RST_TOUCH_PIN, uint8_t active_level);
button_handle_t button_create_button_gpio(const button_config_t *config);
err_code_t button_register_callback(button_handle_t btn_handle, button_event_t event, button_cb_t cb, void *usr_data);
button_event_t button_get_event(button_handle_t btn_handle);
err_code_t button_delete_handle(button_handle_t btn_handle);

void rd_button_cb(void);

#endif /* BUTTON_H_ */
