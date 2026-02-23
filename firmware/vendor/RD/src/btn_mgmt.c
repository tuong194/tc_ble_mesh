/*
 * btn_mgmt.c
 *
 *  Created on: Feb 10, 2026
 *      Author: PC5
 */

#include "../inc/utils.h"
#include "../inc/btn_mgmt.h"
#include "../inc/rd_err.h"

static button_handle_t btn_handler[MAX_NUM_BUTTON];
static void board_button_event_cb(void *arg, void *data);
static event_post_cb_t btn_post_event_cb;

err_code_t btn_mgmt_gpio_config(void){
	err_code_t ret = CODE_OK;
    const button_gpio_config_t button_gpio[MAX_NUM_BUTTON] = {
        {
            .gpio_num = BUTTON_GPIO_PIN,
            .active_level = BUTTON_ACTIVE_LEVEL,
            .disable_pull = false,
        }
    };

    button_config_t btn_config[MAX_NUM_BUTTON] = {
        {
            .press_time = CONFIG_PRESS_TIME_MS,
            .keep_time = CONFIG_KEEP_TIME_MS,
            .long_keep_time = CONFIG_LONG_KEEP_TIME_MS,
            .button_gpio_config = button_gpio[0]
        }
    };
    btn_handler[0] = button_create_button_gpio(&btn_config[0]);
    if(btn_handler[0] == NULL) ret = CODE_ERR;

    ret += button_register_callback(btn_handler[0], BUTTON_EVENT_PRESS, board_button_event_cb, NULL);
    ret += button_register_callback(btn_handler[0], BUTTON_EVENT_LONG_PRESS, board_button_event_cb, NULL);
    ret += button_register_callback(btn_handler[0], BUTTON_EVENT_RELEASE_LONG_PRESS, board_button_event_cb, NULL);
    ret += button_register_callback(btn_handler[0], BUTTON_EVENT_LONG_LONG_PRESS, board_button_event_cb, NULL);
    ret += button_register_callback(btn_handler[0], BUTTON_EVENT_RELEASE_LONG_LONG_PRESS, board_button_event_cb, NULL);

    return ret;
}

static u8 btn_index_pair = 0xff;
static btn_event_id_t evt_post = EVENT_BUTTON_MAX;
static void board_button_event_cb(void *arg, void *data){
    button_handle_t btn_handler = (button_handle_t) arg;
    button_event_t event = button_get_event(btn_handler);
    switch (event)
    {
    case BUTTON_EVENT_PRESS:
        LOGD("button press");
        evt_post = EVENT_BUTTON_PRESS;
        if(btn_post_event_cb) btn_post_event_cb(&evt_post, NULL);
        if(btn_index_pair == 1){
        	LOGD("button delete all k9b");
            evt_post = EVENT_BUTTON_DELETE_ALL_K9B;
            if(btn_post_event_cb) btn_post_event_cb(&evt_post, NULL);
            btn_index_pair = 0xff;
        }
        break;
    case BUTTON_EVENT_LONG_PRESS:{
    	LOGD("button is keeping");
        btn_index_pair = 1;
        evt_post = EVENT_BUTTON_PAIR_K9B;
        if(btn_post_event_cb) btn_post_event_cb(&evt_post, NULL);
        break;
    }
    case BUTTON_EVENT_RELEASE_LONG_PRESS:{
    	LOGD("button release keeping");
        break;
    }
    case BUTTON_EVENT_LONG_LONG_PRESS:{
    	LOGD("button is long keeping");
        evt_post = EVENT_BUTTON_KICK_OUT;
        if(btn_post_event_cb) btn_post_event_cb(&evt_post, NULL);
        break;
    }
    case BUTTON_EVENT_RELEASE_LONG_LONG_PRESS:
    	LOGD("button release long keeping");
        break;

    default:
        break;
    }
}

void btn_mgmt_register_event_handle(event_post_cb_t cb){
    if(cb) btn_post_event_cb = cb;
}
