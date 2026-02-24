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

struct {
	uint32_t U;
	uint32_t I;
	uint32_t P;
	uint32_t P_Consume;
}electrical_param;

static void button_event_handle(void* event, void* usr_data);
struct electrical_param ePar;
static uint32_t P_threshold = 0;
static uint32_t I_threshold = 0;

static uint8_t dev_state = OFF_STATE;

uint32_t aptomat_get_voltage(void) { return ePar.U; }
uint32_t aptomat_get_current(void) { return ePar.I; }
uint32_t aptomat_get_power(void) { return ePar.P; }
uint32_t aptomat_get_power_consume(void) { return ePar.P_Consume; }
void dev_set_threshold_power(uint32_t thres_val) {
	P_threshold = thres_val;
}
void dev_set_threshold_current(uint32_t thres_val) {
	I_threshold = thres_val;
}

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


static void button_event_handle(void* event, void* usr_data) {
	btn_event_id_t event_id = *(btn_event_id_t*)(event);

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

void dev_set_state(uint8_t onoff) {
	if (dev_state == onoff) return;
	dev_state = onoff;
	led_set_state(LED_ONOFF, dev_state);
	relay_set_state(dev_state);
}

uint8_t dev_get_state(void) {
	return dev_state;
}

void task_bl0942_read(void) {
	static u32 last_time_read = 0;
	static P_Consume_ws = 0;
	if (clock_time_ms() - last_time_read >= TIME_CYCLE_READ_MS) {
		uint32_t U_in = bl0942_read_data_unsigned(BL0942_REG_VRMS);
		uint32_t I_in = bl0942_read_data_unsigned(BL0942_REG_IRMS);
		s32 P_in = bl0942_read_data_signed(BL0942_REG_WATT);

		float Uhd = (float)U_in * MULTIPLIER_U; if (Uhd > 2.0f) Uhd = Uhd - 2.0f;
		float Ihd = (float)I_in * MULTIPLIER_I;
		float Phd = (float)P_in * MULTIPLIER_P;

		if (Phd < 0) Phd = 0;

		ePar.U = Uhd * 100;
		ePar.I = Ihd * 100;
		ePar.P = Phd * 100;

		P_Consume_ws += ePar.P * (TIME_CYCLE_READ_MS / 1000);
		ePar.P_Consume = P_Consume_ws / 3600;

		LOGI("U_in: %d.%02d V, I_in: %d.%02d A, P_in: %d.%02d W\n", ePar.U / 100, ePar.U % 100, ePar.I / 100, ePar.I % 100, ePar.P / 100, ePar.P % 100);


		last_time_read = clock_time_ms();
	}
}
