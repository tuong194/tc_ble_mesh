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
#include "../inc/rd_err.h"
#include "../inc/mess_handle.h"

#include "../inc/rd_flash.h"

#define BIT_0   (1<<0)
#define BIT_1	(1<<1)
#define BIT_2	(1<<2)

#define BIT_CHECK_ERROR_POWER     BIT_0
#define BIT_CHECK_ERROR_CURRENT   BIT_1

#define BIT_CHECK_BIND_ALL        BIT_0
#define BIT_CHECK_ENCRYPT_DONE    BIT_1
#define BIT_CHECK_ENCRYPT_FAIL    BIT_2

struct electrical_param{
	uint32_t U;
	uint32_t I;
	uint32_t P;
	uint32_t P_Consume;
	uint8_t  is_safe;
};

typedef struct{
	uint8_t  header[4];
	uint32_t P_threshold;
	uint32_t I_threshold;
	uint8_t  secure;
	uint32_t TIME_CYCLE_READ_MS;
	uint8_t  MAX_CYCLE_DETECT_ERROR_I;
	uint8_t  MAX_CYCLE_DETECT_ERROR_P;
}flash_data_t;

static flash_data_t flash_data;
static struct electrical_param ePar;
static uint8_t dev_state = OFF_STATE;

// static uint32_t TIME_CYCLE_READ_MS = 10000;
// static uint8_t MAX_CYCLE_DETECT_ERROR_I = 5;
// static uint8_t MAX_CYCLE_DETECT_ERROR_P = 5;

// static uint8_t flag_check_mess_secure = 0;
// _Bool flag_provision = FALSE;
int vrs_time_bind_all = 0;
int vrs_time_aes_err = 0;



static void aptomat_read_electrical_param(void);
static err_code_t aptomat_check_error(uint32_t I_in, uint32_t P_in, uint32_t I_threshold, uint32_t P_threshold);
static void button_event_handle(void* event, void* usr_data);
static void check_secure_event_handle(void* event, void* usr_data);

uint32_t aptomat_get_voltage(void) { return ePar.U; }
uint32_t aptomat_get_current(void) { return ePar.I; }
uint32_t aptomat_get_power(void) { return ePar.P; }
uint32_t aptomat_get_power_consume(void) { return ePar.P_Consume; }


void rd_dev_clear_secure(void){
	flash_data.secure = 0;
	rd_write_flash_common();
}

static inline uint8_t get_provision_secure(void){
	return flash_data.secure;
}

void dev_set_time_cycle_read_param_electrical(uint32_t time_ms){
	flash_data.TIME_CYCLE_READ_MS = time_ms;
	rd_write_flash_common();
}

void dev_set_max_num_detect_err_current(uint8_t num){
	flash_data.MAX_CYCLE_DETECT_ERROR_I = num;
	rd_write_flash_common();
}

void dev_set_max_num_detect_err_power(uint8_t num){
	flash_data.MAX_CYCLE_DETECT_ERROR_P = num;
	rd_write_flash_common();
}

void dev_set_threshold_power(uint32_t thres_val) {
	flash_data.P_threshold = thres_val;
	rd_write_flash_common();
}
void dev_set_threshold_current(uint32_t thres_val) {
	flash_data.I_threshold = thres_val;
	rd_write_flash_common();
}

void controller_init(void) {
	heap_init();
	led_init_gpio();
	relay_init_gpio();
	// init state default
	dev_state = ON_STATE;
	led_set_state(LED_ONOFF, dev_state);
	led_set_state(LED_SIGNAL, OFF_STATE);
	relay_set_state(dev_state);

	if(get_provision_state() == STATE_DEV_PROVED){
		led_set_state(LED_SIGNAL, ON_STATE);
	}

	btn_mgmt_gpio_config();
	btn_mgmt_register_event_handle(button_event_handle);
	rd_register_event_secure(check_secure_event_handle);

	ePar.is_safe = RD_SAFETY;
}

extern void kick_out(int led_en);
static void button_event_handle(void* event, void* usr_data) {
	btn_event_id_t event_id = *(btn_event_id_t*)(event);

	switch (event_id) {
	case EVENT_BUTTON_PRESS:
		LOGI("[controller] btn press");
		uint8_t onoff = dev_get_state();
		dev_set_state(!onoff);
		break;
	case EVENT_BUTTON_PAIR_K9B:
		LOGI("[controller] btn pair k9b");
		break;
	case EVENT_BUTTON_DELETE_ALL_K9B:
		LOGI("[controller] delete all k9b");
		break;
	case EVENT_BUTTON_KICK_OUT:
		LOGW("[controller] kickout");
		kick_out(0);
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

void task_bl0942(void) {
	static u32 last_time_read = 0;
	if (clock_time_ms() - last_time_read >= flash_data.TIME_CYCLE_READ_MS) {
		aptomat_read_electrical_param();
		err_code_t ret = aptomat_check_error(ePar.I, ePar.P, flash_data.I_threshold, flash_data.P_threshold);
		if(ret == ERR_DETECT_ELECTRICAL && ePar.is_safe == RD_SAFETY){
			ePar.is_safe = RD_ERROR;
			dev_set_state(OFF_STATE);
		}else if(ret == CODE_OK){
			ePar.is_safe = RD_SAFETY;
		}
		last_time_read = clock_time_ms();
	}
}

static void aptomat_read_electrical_param(void){
	static u64 P_Consume_ws = 0;
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

	P_Consume_ws += ePar.P * (flash_data.TIME_CYCLE_READ_MS / 1000); // Ws
	ePar.P_Consume = P_Consume_ws / 3600; //kWh

	LOGI("U_in: %d.%02d V, I_in: %d.%02d A, P_in: %d.%02d W",\
		ePar.U / 100, ePar.U % 100, ePar.I / 100, ePar.I % 100, ePar.P / 100, ePar.P % 100);

}

static err_code_t aptomat_check_error(uint32_t I_in, uint32_t P_in, uint32_t I_threshold, uint32_t P_threshold){
	static uint8_t flag_check_err = 0;
	static s8 count_check_err_current = 0;
	static s8 count_check_err_power = 0;
	if(I_threshold > 0){
		if(I_in > I_threshold){
			count_check_err_current++;
			LOGD("current too high, count: %d", count_check_err_current);
			if(count_check_err_current >= flash_data.MAX_CYCLE_DETECT_ERROR_I){
				count_check_err_current = flash_data.MAX_CYCLE_DETECT_ERROR_I;
				flag_check_err |= BIT_CHECK_ERROR_CURRENT;
			}
		}else{
			if(count_check_err_current > 1) count_check_err_current--;
			if(count_check_err_current == 0){
				flag_check_err &= ~BIT_CHECK_ERROR_CURRENT;
				LOGI("current is safety");
			}
		}
	}

	if(P_threshold > 0){
		if(P_in > P_threshold){
			count_check_err_power++;
			LOGD("power too high, count: %d", count_check_err_power);
			if(count_check_err_power >= flash_data.MAX_CYCLE_DETECT_ERROR_P){
				count_check_err_power = flash_data.MAX_CYCLE_DETECT_ERROR_P;
				flag_check_err |= BIT_CHECK_ERROR_POWER;
			}
		}else{
			if(count_check_err_power > 1) count_check_err_power--;
			if(count_check_err_power == 0){
				flag_check_err &= ~BIT_CHECK_ERROR_POWER;
				LOGI("power is safety");
			}
		}
	}


	if(flag_check_err & BIT_CHECK_ERROR_POWER || flag_check_err & BIT_CHECK_ERROR_CURRENT){
		LOGE("detect error: %02x, shut down device now", flag_check_err);
		return ERR_DETECT_ELECTRICAL;
	}else{
//		LOGD("device is safety");
		return CODE_OK;
	}
	return CODE_OK;
}


/**********************************************************
				      AES SECURE 
***********************************************************/
void task_check_kick_out(void){
	uint8_t flag_check_mess_secure = get_provision_secure();
	if(flag_check_mess_secure & BIT_CHECK_BIND_ALL){
		if(flag_check_mess_secure & BIT_CHECK_ENCRYPT_FAIL){
			if(clock_time_s() - vrs_time_bind_all > 10){
				kick_out(0);
			}
		}else if( !(flag_check_mess_secure & (BIT_CHECK_ENCRYPT_FAIL | BIT_CHECK_ENCRYPT_DONE)) ){
			if(clock_time_s() - vrs_time_bind_all > 120){
				kick_out(0);
			}
		}
	}
}

static void check_secure_event_handle(void* event, void* usr_data){
	secure_event_t event_id = *(secure_event_t *)event;
	switch (event_id)
	{
	case EVENT_SECURE_BIND_ALL:
	{
		flash_data.secure |= BIT_CHECK_BIND_ALL;
		// blink led
		vrs_time_bind_all = clock_time_s();
		if(vrs_time_bind_all >= 0xfffffffe) vrs_time_bind_all = 0;
		break;
	}
	case EVENT_SECURE_ENCRYPT_DONE:
		flash_data.secure |= BIT_CHECK_ENCRYPT_DONE;	
		break;
	case EVENT_SECURE_ENCRYPT_FAIL:
	{
		flash_data.secure |= BIT_CHECK_ENCRYPT_FAIL;	
		vrs_time_aes_err = clock_time_s();
		if(vrs_time_aes_err >= 0xfffffffe) vrs_time_aes_err = 0;
		break;
	}
	
	default:
		return;
		// break;
	}	

	rd_write_flash_common();
} 

void rd_write_flash_common(void){
	flash_erase_sector(RD_FLASH_ADDR_COMMON);
	flash_write_page(RD_FLASH_ADDR_COMMON, sizeof(flash_data_t), (unsigned char *) (&flash_data));	
}

static void rd_init_flash_common_default(void){
	flash_data.header[0] != FLASH_HEADER_1;
	flash_data.header[1] != FLASH_HEADER_2;
	flash_data.header[2] != FLASH_HEADER_1;
	flash_data.header[3] != FLASH_HEADER_2;
	flash_data.secure = 0;
	flash_data.I_threshold = I_THRESHOLD_DEFAULT; 
	flash_data.P_threshold = P_THRESHOLD_DEFAULT;
	flash_data.TIME_CYCLE_READ_MS = TIME_CYCLE_READ_MS_DF;
	flash_data.MAX_CYCLE_DETECT_ERROR_I = MAX_CYCLE_DETECT_ERROR_I_DF;
	flash_data.MAX_CYCLE_DETECT_ERROR_P = MAX_CYCLE_DETECT_ERROR_P_DF;

	rd_write_flash_common();

}

void rd_init_flash_common(void){
	flash_read_page(RD_FLASH_ADDR_COMMON, sizeof(flash_data_t), (unsigned char *)(&flash_data));
	if(flash_data.header[0] != FLASH_HEADER_1 && flash_data.header[1] != FLASH_HEADER_2 && \
	   flash_data.header[2] != FLASH_HEADER_1 && flash_data.header[3] != FLASH_HEADER_2){
		rd_init_flash_common_default();
	}

	//en secure
	if(get_provision_state() == STATE_DEV_PROVED){
		uint8_t flag_check_mess_secure = get_provision_secure();
		if(!(flag_check_mess_secure & BIT_CHECK_ENCRYPT_DONE)){
			kick_out(0);
		}
	}
}

