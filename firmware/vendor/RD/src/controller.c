/*
 * controller.c
 *
 *  Created on: Feb 11, 2026
 *      Author: PC5
 */

#include "../../common/system_time.h"
#include "../inc/controller.h"
#include "../inc/btn_mgmt.h"
#include "../inc/rd_output.h"
#include "../inc/utils.h"
#include "../inc/bl0942.h"

static void button_event_handle(void *event, void *usr_data);

static uint8_t dev_state = OFF_STATE;

void controller_init(void) {
	heap_init();
	led_init_gpio();
	relay_init_gpio();

	btn_mgmt_gpio_config();
	btn_mgmt_register_event_handle(button_event_handle);

	// init state default
	dev_state = OFF_STATE;
	led_set_state(LED_ONOFF, dev_state);
	led_set_state(LED_SIGNAL, dev_state);
	relay_set_state(dev_state);
}


static void button_event_handle(void *event, void *usr_data) {
	btn_event_id_t event_id = *(btn_event_id_t *) (event);

	switch (event_id) {
	case EVENT_BUTTON_PRESS:
		LOGI("[controller] btn press");
		break;
	case EVENT_BUTTON_PAIR_K9B:
		LOGI("[controller] btn pair k9b");
		break;
	case EVENT_BUTTON_DELETE_ALL_K9B:
		LOGI("[controller] delete all k9b");
		break;
	case EVENT_BUTTON_KICK_OUT:
		LOGW("[controller] kickout");
		break;
	default:
		break;
	}
}

void dev_set_state(uint8_t onoff){
	if(dev_state == onoff) return;
	dev_state = onoff;
	led_set_state(LED_ONOFF, dev_state);
	relay_set_state(dev_state);
}

uint8_t dev_get_state(void){
	return dev_state;
}

void task_bl0942_read(void) {
	static u32 last_time_read = 0;
	if (clock_time_ms() - last_time_read >= 5000) {
		uint32_t U_in = bl0942_read_data_unsigned(BL0942_REG_VRMS);
		uint32_t I_in = bl0942_read_data_unsigned(BL0942_REG_IRMS);
		s32 P_in = bl0942_read_data_signed(BL0942_REG_WATT);

		float Uhd = (float) U_in * MULTIPLIER_U; if(Uhd > 2.0f) Uhd = Uhd - 2.0f;
		float Ihd = (float) I_in * MULTIPLIER_I;
		float Phd = (float) P_in * MULTIPLIER_P;

		if (Phd < 0) Phd = 0;

		/* test */
		//			float Uhd = 123.6789;
		//			float Ihd = 568.6789;
		//			float Phd = 46.46554;

		uint32_t U_log = Uhd * 1000;
		uint32_t I_log = Ihd * 1000;
		uint32_t P_log = Phd * 1000;

		LOGI("U_in: %d.%03d V, I_in: %d.%03d A, P_in: %d.%03d W\n", U_log/1000, U_log%1000, I_log/1000, I_log%1000, P_log/1000, P_log%1000);

		last_time_read = clock_time_ms();
	}
}
