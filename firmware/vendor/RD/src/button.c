/*
 * button.c
 *
 *  Created on: Feb 10, 2026
 *      Author: PC5
 */

#include "tl_common.h"

#include "../inc/button.h"
#include "../inc/utils.h"


#define TICK_INTERVAL    10 //ms

#define TICKS_TIME_PRESS_DEFAULT      (100/TICK_INTERVAL)
#define TICKS_TIME_KEEP_DEFAULT       (1000/TICK_INTERVAL)
#define TICKS_TIME_LONG_KEEP_DEFAULT  (5000/TICK_INTERVAL)


#define TIME_TO_TICKS(time, tick_default) ((0==(time))?tick_default:(((time)/TICK_INTERVAL)<tick_default)?tick_default:((time)/TICK_INTERVAL))

#define CALL_EVENT_CB(ev)                                                   \
    if (btn->cb_info[ev]) {                                                 \
        for (int i = 0; i < btn->size[ev]; i++) {                           \
            btn->cb_info[ev][i].cb(btn, btn->cb_info[ev][i].usr_data);      \
        }                                                                   \
    }                                                                       \

typedef struct {
    button_cb_t cb;
    void *usr_data;
    // button_event_data_t event_data;
} button_cb_info_t;

typedef struct Button{
    uint32_t ticks;
    uint16_t press_ticks;
    uint16_t keep_ticks;
    uint16_t long_keep_ticks;
    uint8_t active_level;
    uint8_t (*hal_button_get_Level)(void *hardware_data);
    void     *hardware_data;
    button_event_t event;
    button_cb_info_t *cb_info[BUTTON_EVENT_MAX];
    u8               size[BUTTON_EVENT_MAX];
    struct Button *next;
}button_dev_t;

//button handle list head.
static button_dev_t *g_head_handle = NULL;
static bool g_is_timer_running = false;

//static void rd_button_cb(void);
static void button_handler(button_dev_t *btn);

void button_reset_touch_pin_config(GPIO_PinTypeDef RST_TOUCH_PIN, uint8_t active_level){
	gpio_set_func(RST_TOUCH_PIN ,AS_GPIO);
	gpio_set_output_en(RST_TOUCH_PIN, 1);
	gpio_set_input_en(RST_TOUCH_PIN ,0);
	gpio_write(RST_TOUCH_PIN, active_level);
}

err_code_t button_gpio_init(const button_gpio_config_t *config){
    if(config == NULL){
        return ERR_INVALID_ARG;
    }

	gpio_set_func(config->gpio_num ,AS_GPIO);
	gpio_set_output_en(config->gpio_num, 0);
	gpio_set_input_en(config->gpio_num ,1);

    if (config->disable_pull) {
    	gpio_setup_up_down_resistor(config->gpio_num, PM_PIN_UP_DOWN_FLOAT);
    } else {
        if (config->active_level) { //pull up
        	gpio_setup_up_down_resistor(config->gpio_num, PM_PIN_PULLDOWN_100K);
        } else {
        	gpio_setup_up_down_resistor(config->gpio_num, PM_PIN_PULLUP_10K);
        }
    }
    return CODE_OK;
}

err_code_t button_gpio_deinit(GPIO_PinTypeDef gpio_num)
{
    gpio_shutdown(gpio_num);
    return CODE_OK;
}

static inline uint8_t button_gpio_get_key_level(void *gpio_num)
{
	return (uint8_t)gpio_read((GPIO_PinTypeDef)gpio_num);
}

static button_dev_t *button_create_com(uint8_t active_level, uint8_t (*hal_button_get_Level)(void *hardware_data), void *hardware, uint16_t press_ticks, uint16_t keep_ticks, uint16_t long_keep_ticks){
    button_dev_t *btn = (button_dev_t *) rd_calloc(1, sizeof(button_dev_t));
    if(btn == NULL){
        LOGE("button device calloc FAIL");
    }
    btn->event = BUTTON_EVENT_NONE_PRESS;
    btn->active_level = active_level;
    btn->hardware_data = hardware;
    btn->hal_button_get_Level = hal_button_get_Level;
    btn->press_ticks = press_ticks;
    btn->keep_ticks = keep_ticks;
    btn->long_keep_ticks = long_keep_ticks;
    // Add to LIST
    btn->next = g_head_handle;
    g_head_handle = btn;

    return btn;
}

static err_code_t button_delete_com(button_dev_t *btn){
    if(!btn){
        LOGE("Pointer of handle is invalid");
        return ERR_INVALID_ARG;
    }
    button_dev_t **curr;
    for (curr = &g_head_handle; *curr;) {
        button_dev_t *entry = *curr;
        if (entry == btn) {
            *curr = entry->next;
            rd_free(entry);
        } else {
            curr = &entry->next;
        }
    }

    uint8_t num_handle = 0;
    button_dev_t *head = g_head_handle;
    while(head){
        head = head->next;
        num_handle ++;
    }

    LOGI("remain btn handle : %d", num_handle);

    if(num_handle == 0 && g_is_timer_running == true){
    	//stop timer
        g_is_timer_running = false;
    }
    return CODE_OK;
}

err_code_t button_delete_handle(button_handle_t btn_handle){
    button_dev_t *btn = (button_dev_t *)btn_handle;
    if(!btn){
        LOGE("Pointer of handle is invalid");
        return ERR_INVALID_ARG;
    }
    err_code_t ret = button_gpio_deinit((GPIO_PinTypeDef)(btn->hardware_data));
    if(ret != CODE_OK){
        LOGE("button deinit fail");
        return CODE_ERR;
    }
    for (size_t i = 0; i < BUTTON_EVENT_MAX; i++)
    {
        if(btn->cb_info[i]) rd_free(btn->cb_info[i]);
    }

    return button_delete_com(btn);
}

button_handle_t button_create_button_gpio(const button_config_t *config){
    err_code_t ret = CODE_OK;

    button_dev_t *btn = NULL;
    uint16_t press_ticks = TIME_TO_TICKS(config->press_time, TICKS_TIME_PRESS_DEFAULT);
    uint16_t keep_ticks = TIME_TO_TICKS(config->keep_time, TICKS_TIME_KEEP_DEFAULT);
    uint16_t long_keep_ticks = TIME_TO_TICKS(config->long_keep_time, TICKS_TIME_LONG_KEEP_DEFAULT);
    const button_gpio_config_t *cfg = &(config->button_gpio_config);
    ret = button_gpio_init(cfg);
    if(ret != CODE_OK){
        LOGE("button gpio init FAIL");
        return NULL;
    }
    LOGI("press %d, keeping: %d, long keeping: %d", press_ticks, keep_ticks, long_keep_ticks);
    btn = button_create_com(cfg->active_level,button_gpio_get_key_level, (void *)cfg->gpio_num, press_ticks, keep_ticks, long_keep_ticks);
    if(!g_is_timer_running){
        g_is_timer_running = true;
        // start timer
    }
    return btn;
}

err_code_t button_register_callback(button_handle_t btn_handle, button_event_t event, button_cb_t cb, void *usr_data){
    if(btn_handle == NULL) return ERR_INVALID_ARG;
    button_dev_t *btn = (button_dev_t *) btn_handle;
    if(event == BUTTON_EVENT_LONG_PRESS || event == BUTTON_EVENT_RELEASE_LONG_PRESS){
        if(btn->keep_ticks <= btn->press_ticks){
            LOGE("keep time is less than press time");
            return ERR_SET_VALUE;
        }
    }
    if(event == BUTTON_EVENT_LONG_LONG_PRESS || event == BUTTON_EVENT_RELEASE_LONG_LONG_PRESS){
        if(btn->long_keep_ticks <= btn->keep_ticks){
            LOGE("long keep time is less than keep time");
            return ERR_SET_VALUE;
        }
    }
    if(!btn->cb_info[event]){
        btn->cb_info[event] = rd_calloc(1, sizeof(button_cb_info_t));
        if(btn->cb_info[event] == NULL){
            LOGE("calloc call back fail");
            return CODE_ERR;
        }
    }else{
        button_cb_info_t *p = rd_realloc(btn->cb_info[event], sizeof(button_cb_info_t) * (btn->size[event] +1));
        if(p == NULL){
            LOGE("p calloc call back fail");
            return CODE_ERR;
        }
        btn->cb_info[event] = p;
    }

    btn->cb_info[event][btn->size[event]].cb = cb;
    btn->cb_info[event][btn->size[event]].usr_data = usr_data;
    btn->size[event]++;

    return CODE_OK;
}

button_event_t button_get_event(button_handle_t btn_handle)
{
    button_dev_t *btn = (button_dev_t *) btn_handle;
    return btn->event;
}

void rd_button_cb(void)
{
    button_dev_t *node;
    for(node = g_head_handle;node; node = node->next){
        button_handler(node);
    }
}

static void button_handler(button_dev_t *btn){
    uint8_t gpio_level = btn->hal_button_get_Level(btn->hardware_data);
    if(gpio_level == btn->active_level){
        btn->ticks++;
    }else{
        btn->ticks = 0;
    }

    if(btn->event == BUTTON_EVENT_NONE_PRESS && btn->ticks >= btn->press_ticks){
        btn->event = BUTTON_EVENT_PRESS;
    }else if(btn->event == BUTTON_EVENT_PRESS &&  btn->ticks >= btn->keep_ticks){
        btn->event = BUTTON_EVENT_LONG_PRESS;
        //cb keeping
        CALL_EVENT_CB(BUTTON_EVENT_LONG_PRESS);
    }else if(btn->event == BUTTON_EVENT_LONG_PRESS &&  btn->ticks >= btn->long_keep_ticks){
        btn->event = BUTTON_EVENT_LONG_LONG_PRESS;
        //cb long keeping
        CALL_EVENT_CB(BUTTON_EVENT_LONG_LONG_PRESS);
    }else if(btn->ticks == 0){
        if(btn->event == BUTTON_EVENT_LONG_LONG_PRESS){
            btn->event = BUTTON_EVENT_RELEASE_LONG_LONG_PRESS;
            //cb release long keeping
            CALL_EVENT_CB(BUTTON_EVENT_RELEASE_LONG_LONG_PRESS);
        }else if(btn->event == BUTTON_EVENT_LONG_PRESS){
            btn->event = BUTTON_EVENT_RELEASE_LONG_PRESS;
            //cb release keeping
            CALL_EVENT_CB(BUTTON_EVENT_RELEASE_LONG_PRESS);
        }else if(btn->event == BUTTON_EVENT_PRESS){
            //cb press
            CALL_EVENT_CB(BUTTON_EVENT_PRESS);
        }
        btn->event = BUTTON_EVENT_NONE_PRESS;
    }
}
