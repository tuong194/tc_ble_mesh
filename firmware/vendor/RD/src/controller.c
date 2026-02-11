/*
 * controller.c
 *
 *  Created on: Feb 11, 2026
 *      Author: PC5
 */
#include "../inc/controller.h"
#include "../inc/btn_mgmt.h"
#include "../inc/rd_output.h"
#include "../inc/utils.h"

static void button_event_handle(void *event, void *usr_data);
static uint8_t state = 0;

void controller_init(void){
    heap_init();
    led_init_gpio();
    relay_init_gpio();

    btn_mgmt_gpio_config();
    btn_mgmt_register_event_handle(button_event_handle);

    state = led_get_state(LED_ONOFF);
}

static void button_event_handle(void *event, void *usr_data){
    btn_event_id_t event_id = *(btn_event_id_t *)(event);

    switch (event_id)
    {
    case EVENT_BUTTON_PRESS:

        LOGI("[controller] btn press");
        state = !state;
        led_set_state(LED_ONOFF, state);
        relay_set_state(state);
        break;
    case EVENT_BUTTON_PAIR_K9B:
        LOGI("[controller] btn pair k9b");
        break;
    case EVENT_BUTTON_DELETE_ALL_K9B:
        LOGI("[controller] delete all k9b");
        break;
    default:
        break;
    }
}
