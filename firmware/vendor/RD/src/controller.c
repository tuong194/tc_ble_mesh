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

#define BIT_0 (1 << 0)
#define BIT_1 (1 << 1)
#define BIT_2 (1 << 2)
#define BIT_3 (1 << 3)
#define BIT_4 (1 << 4)
#define BIT_5 (1 << 5)
#define BIT_6 (1 << 6)
#define BIT_7 (1 << 7)

#define BIT_CHECK_ERROR_VOL_TOO_HIGH BIT_0
#define BIT_CHECK_ERROR_VOL_TOO_LOW  BIT_1
#define BIT_CHECK_ERROR_CUR_TOO_HIGH BIT_2
#define BIT_CHECK_ERROR_CUR_TOO_LOW  BIT_3
#define BIT_CHECK_ERROR_POWER        BIT_5

#define BIT_CHECK_BIND_ALL     BIT_0
#define BIT_CHECK_ENCRYPT_DONE BIT_1
#define BIT_CHECK_ENCRYPT_FAIL BIT_2

#define TIME_OUT_BIND_ALL_SECOND 120
#define TIME_OUT_AES_FAIL_SECOND 10

typedef struct
{
	uint32_t U;
	uint32_t I;
	uint32_t P;
	uint32_t P_Consume;
	uint8_t error;
	uint8_t is_safe;
} electrical_param;

typedef struct
{
	uint8_t header[4];
	uint8_t secure;
	uint8_t MAX_CYCLE_DETECT_ERROR_U;
	uint8_t MAX_CYCLE_DETECT_ERROR_I;
	uint8_t MAX_CYCLE_DETECT_ERROR_P;
	uint32_t TIME_CYCLE_READ_MS;
	uint32_t P_threshold;
	uint32_t I_threshold_high;
	uint32_t I_threshold_low;
	uint32_t U_threshold_high;
	uint32_t U_threshold_low;
} flash_data_t;

static flash_data_t flash_data;
static electrical_param ePar;
static uint8_t dev_state = OFF_STATE;

int vrs_time_bind_all = 0;
int vrs_time_aes_err = 0;

static _Bool is_ota = false;

static err_code_t aptomat_check_error_voltage(uint32_t U_in, uint32_t U_threshold_low, uint32_t U_threshold_high, uint8_t MAX_NUM_DETECT, uint8_t *check_safe);
static err_code_t aptomat_check_error_current(uint32_t I_in, uint32_t I_threshold_low, uint32_t I_threshold_high, uint8_t MAX_NUM_DETECT, uint8_t *check_safe);
static err_code_t aptomat_check_error_power(uint32_t P_in, uint32_t P_threshold, uint8_t MAX_NUM_DETECT, uint8_t *check_safe);

static void aptomat_read_electrical_param(electrical_param *par, uint32_t time_cycle_read_ms);
static void button_event_handle(void *event, void *usr_data);
static void check_secure_event_handle(void *event, void *usr_data);

uint32_t aptomat_get_voltage(void) { return ePar.U; }
uint32_t aptomat_get_current(void) { return ePar.I; }
uint32_t aptomat_get_power(void) { return ePar.P; }
uint32_t aptomat_get_power_consume(void) { return ePar.P_Consume; }

void rd_ota_start(void)
{
	LOGI("OTA start ...");
	is_ota = true;

	vrs_time_bind_all = clock_time_s();
	uint32_t time_temp = 0xffffffff - vrs_time_bind_all;
	if (time_temp <= TIME_OUT_BIND_ALL_SECOND)
	{
		vrs_time_bind_all = TIME_OUT_BIND_ALL_SECOND - time_temp;
	}
}

void rd_ota_end(uint8_t result)
{
	is_ota = false;
	if (result)
	{
		LOGI("OTA success ...");
	}
	else
	{
		LOGI("OTA fail, start reboot");
	}
	rd_show_ota_result(result);
}

void rd_show_ota_result(uint8_t result)
{
	if (result)
	{
		LOGI("OTA success, show result ...");
		led_mgmt_set_blink_delay(LED_SIGNAL, 7, 300);
	}
	else
	{
		LOGI("OTA fail, show result ...");
		led_mgmt_set_blink_delay(LED_SIGNAL, 3, 300);
	}
}

static inline uint8_t get_provision_secure(void)
{
	return flash_data.secure;
}

void dev_set_time_cycle_read_param_electrical(uint32_t time_ms)
{
	flash_data.TIME_CYCLE_READ_MS = time_ms;
	rd_write_flash_common();
}

void dev_set_max_num_detect_err_voltage(uint8_t num)
{
	flash_data.MAX_CYCLE_DETECT_ERROR_U = num;
	rd_write_flash_common();
}

void dev_set_max_num_detect_err_current(uint8_t num)
{
	flash_data.MAX_CYCLE_DETECT_ERROR_I = num;
	rd_write_flash_common();
}

void dev_set_max_num_detect_err_power(uint8_t num)
{
	flash_data.MAX_CYCLE_DETECT_ERROR_P = num;
	rd_write_flash_common();
}

void dev_set_threshold_power(uint32_t thres_val)
{
	flash_data.P_threshold = thres_val;
	rd_write_flash_common();
}

void dev_set_threshold_current_high(uint32_t thres_val)
{
	flash_data.I_threshold_high = thres_val;
	rd_write_flash_common();
}

void dev_set_threshold_current_low(uint32_t thres_val)
{
	flash_data.I_threshold_low = thres_val;
	rd_write_flash_common();
}

void dev_set_threshold_voltage_high(uint32_t thres_val)
{
	flash_data.U_threshold_high = thres_val;
	rd_write_flash_common();
}

void dev_set_threshold_voltage_low(uint32_t thres_val)
{
	flash_data.U_threshold_low = thres_val;
	rd_write_flash_common();
}

void controller_init(void)
{
	heap_init();
	led_init_gpio();
	relay_init_gpio();
	led_set_state(LED_ONOFF, dev_state);
	relay_set_state(dev_state);

	if (get_provision_state() == STATE_DEV_PROVED)
	{
		led_set_state(LED_SIGNAL, ON_STATE);
	}
	else
	{
		led_set_state(LED_SIGNAL, OFF_STATE);
	}

	btn_mgmt_gpio_config();
	btn_mgmt_register_event_handle(button_event_handle);
	rd_register_event_secure(check_secure_event_handle);

	ePar.is_safe = RD_SAFETY;
	ePar.error = 0;
}

extern void kick_out(int led_en);
static void button_event_handle(void *event, void *usr_data)
{
	btn_event_id_t event_id = *(btn_event_id_t *)(event);

	switch (event_id)
	{
	case EVENT_BUTTON_PRESS:
	{
		uint8_t onoff = dev_get_state();
		uint16_t dst_addr = rd_get_gateway_addr();
		uint8_t rsp_buf[2];
//		if (ePar.is_safe != RD_SAFETY && onoff == OFF_STATE)
//		{
//			LOGW("on, set safety");
//			ePar.is_safe = RD_SAFETY;
//		}
		dev_set_state(!onoff);
		rsp_buf[0] = !onoff;
		rsp_buf[1] = 0;

		mesh_tx_cmd2normal_primary(0x0482, rsp_buf, 2, dst_addr, 0); //generic onoff
		break;
	}
	case EVENT_BUTTON_PAIR_K9B:
		// LOGI("[controller] btn pair k9b");
		break;
	case EVENT_BUTTON_DELETE_ALL_K9B:
		// LOGI("[controller] delete all k9b");
		break;
	case EVENT_BUTTON_KICK_OUT:
		LOGW("[controller] kick out");
		led_mgmt_set_blink_delay(LED_SIGNAL, 11, 150);
		kick_out(0);
		break;
	default:
		break;
	}
}

void dev_set_state(uint8_t onoff)
{
	if (dev_state == onoff)
		return;
	LOGI("set state on/off %d", onoff);
	dev_state = onoff;
	led_set_state(LED_ONOFF, dev_state);
	relay_set_state(dev_state);
}

uint8_t dev_get_state(void)
{
	return dev_state;
}

void dev_get_onoff_last(uint8_t onoff)
{
	LOGI("last onoff: %d", onoff);
	dev_state = onoff;
}

void task_bl0942(void)
{
	static u32 last_time_read = 0;
	static bool is_run = false;
	if (clock_time_ms() < last_time_read)
	{
		uint32_t time_temp = 0xffffffff - last_time_read;
		if (clock_time_ms() + time_temp >= flash_data.TIME_CYCLE_READ_MS)
		{
			is_run = true;
			last_time_read = clock_time_ms();
		}
	}
	else
	{
		if (clock_time_ms() - last_time_read >= flash_data.TIME_CYCLE_READ_MS)
		{
			is_run = true;
			last_time_read = clock_time_ms();
		}
	}
	if (is_run)
	{
		is_run = false;
		aptomat_read_electrical_param(&ePar, flash_data.TIME_CYCLE_READ_MS);

		err_code_t err = aptomat_check_error_power(ePar.P, flash_data.P_threshold, flash_data.MAX_CYCLE_DETECT_ERROR_P, &ePar.error);
		if(err == POWER_ERR_BACK_TO_NORMAL) //normal
		{

		}else if(err == ERR_POWER)
		{

		}

		err = aptomat_check_error_current(ePar.I, flash_data.I_threshold_low, flash_data.I_threshold_high, flash_data.MAX_CYCLE_DETECT_ERROR_I, &ePar.error);
		if(err == CURRENT_HIGH_BACK_TO_NORMAL || CURRENT_LOW_BACK_TO_NORMAL) //normal
		{

		}else if(err == ERR_CUR_TOO_HIGH)
		{

		}else if(err == ERR_CUR_TOO_LOW)
		{

		}

		err = aptomat_check_error_voltage(ePar.U, flash_data.U_threshold_low, flash_data.U_threshold_high, flash_data.MAX_CYCLE_DETECT_ERROR_U, &ePar.error);
		if(err == VOLTAGE_HIGH_BACK_TO_NORMAL || VOLTAGE_LOW_BACK_TO_NORMAL) //normal
		{

		}else if(err == ERR_VOL_TOO_HIGH)
		{

		}else if(err == ERR_VOL_TOO_LOW)
		{

		}		
	}
}

static void aptomat_read_electrical_param(electrical_param *par, uint32_t time_cycle_read_ms)
{
	static u64 P_Consume_ws = 0;
	uint32_t U_in = bl0942_read_data_unsigned(BL0942_REG_VRMS);
	uint32_t I_in = bl0942_read_data_unsigned(BL0942_REG_IRMS);
	s32 P_in = bl0942_read_data_signed(BL0942_REG_WATT);

	float Uhd = (float)U_in * MULTIPLIER_U;
	if (Uhd > 2.0f)
		Uhd = Uhd - 2.0f;
	float Ihd = (float)I_in * MULTIPLIER_I;
	float Phd = (float)P_in * MULTIPLIER_P;

	if (Phd < 0)
		Phd = 0;

	par->U = Uhd * 100;
	par->I = Ihd * 100;
	par->P = Phd * 100;

	P_Consume_ws += par->P * (time_cycle_read_ms / 1000); // Ws
	par->P_Consume = P_Consume_ws / 3600;				  // kWh

	LOGI("U_in: %d.%02d V, I_in: %d.%02d A, P_in: %d.%02d W",
		 par->U / 100, par->U % 100, par->I / 100, par->I % 100, par->P / 100, par->P % 100);
}

static err_code_t aptomat_check_error_voltage(uint32_t U_in, uint32_t U_threshold_low, uint32_t U_threshold_high, uint8_t MAX_NUM_DETECT, uint8_t *check_safe){
	static s8 count_check_vol_too_high = 0;
	static s8 count_check_vol_too_low = 0;

	if(U_threshold_high == 0 || U_threshold_high <= U_threshold_low) return ERR_INVALID_ARG;

	if(U_in >= U_threshold_low && U_in <= U_threshold_high){
		if(*check_safe & BIT_CHECK_ERROR_VOL_TOO_HIGH){
			if(count_check_vol_too_high > 0) count_check_vol_too_high--;
			if(count_check_vol_too_high == 0){
				count_check_vol_too_low = 0;
				*check_safe &= ~BIT_CHECK_ERROR_VOL_TOO_HIGH;
				*check_safe &= ~BIT_CHECK_ERROR_VOL_TOO_LOW;
				LOGD("voltage too high back to normal");
				return VOLTAGE_HIGH_BACK_TO_NORMAL;
			}
		} 
		if(*check_safe & BIT_CHECK_ERROR_VOL_TOO_LOW){
			if(count_check_vol_too_low > 0) count_check_vol_too_low--;
			if(count_check_vol_too_low == 0){
				count_check_vol_too_high = 0;
				*check_safe &= ~BIT_CHECK_ERROR_VOL_TOO_LOW;
				*check_safe &= ~BIT_CHECK_ERROR_VOL_TOO_HIGH;
				LOGD("voltage too low back to normal");
				return VOLTAGE_LOW_BACK_TO_NORMAL;
			}
		}
	}else{
		if(U_in > U_threshold_high){
			if(!(*check_safe & BIT_CHECK_ERROR_VOL_TOO_HIGH)){
				if(count_check_vol_too_high < MAX_NUM_DETECT) count_check_vol_too_high++;
				LOGD("voltage too high [%d]", count_check_vol_too_high);
				if(count_check_vol_too_high == MAX_NUM_DETECT){
					*check_safe |= BIT_CHECK_ERROR_VOL_TOO_HIGH;
					return ERR_VOL_TOO_HIGH;
				}				
			}
		}else if(U_in < U_threshold_low){
			if(!(*check_safe & BIT_CHECK_ERROR_VOL_TOO_LOW)){
				if(count_check_vol_too_low < MAX_NUM_DETECT) count_check_vol_too_low++;
				LOGD("voltage too low [%d]", count_check_vol_too_low);
				if(count_check_vol_too_low == MAX_NUM_DETECT){
					*check_safe |= BIT_CHECK_ERROR_VOL_TOO_LOW;
					return ERR_VOL_TOO_LOW;
				}			
			}
		}
	}
	return CODE_OK;
}

static err_code_t aptomat_check_error_current(uint32_t I_in, uint32_t I_threshold_low, uint32_t I_threshold_high, uint8_t MAX_NUM_DETECT, uint8_t *check_safe){
	static s8 count_check_cur_too_high = 0;
	static s8 count_check_cur_too_low = 0;

	if(I_threshold_high == 0 || I_threshold_high <= I_threshold_low) return ERR_INVALID_ARG;

	if(I_in >= I_threshold_low && I_in <= I_threshold_high){
		if(*check_safe & BIT_CHECK_ERROR_CUR_TOO_HIGH){
			if(count_check_cur_too_high > 0) count_check_cur_too_high--;
			if(count_check_cur_too_high == 0){
				count_check_cur_too_low = 0;
				*check_safe &= ~BIT_CHECK_ERROR_CUR_TOO_HIGH;
				*check_safe &= ~BIT_CHECK_ERROR_CUR_TOO_LOW;
				LOGD("current too high back to normal");
				return CURRENT_HIGH_BACK_TO_NORMAL;
			}
		} 
		if(*check_safe & BIT_CHECK_ERROR_CUR_TOO_LOW){
			if(count_check_cur_too_low > 0) count_check_cur_too_low--;
			if(count_check_cur_too_low == 0){
				count_check_cur_too_high = 0;
				*check_safe &= ~BIT_CHECK_ERROR_CUR_TOO_LOW;
				*check_safe &= ~BIT_CHECK_ERROR_CUR_TOO_HIGH;
				LOGD("current too low back to normal");
				return CURRENT_LOW_BACK_TO_NORMAL;
			}
		}
	}else{
		if(I_in > I_threshold_high){
			if(!(*check_safe & BIT_CHECK_ERROR_CUR_TOO_HIGH)){
				if(count_check_cur_too_high < MAX_NUM_DETECT) count_check_cur_too_high++;
				LOGD("current too high [%d]", count_check_cur_too_high);
				if(count_check_cur_too_high == MAX_NUM_DETECT){
					*check_safe |= BIT_CHECK_ERROR_CUR_TOO_HIGH;
					return ERR_CUR_TOO_HIGH;
				}				
			}
		}else if(I_in < I_threshold_low){
			if(!(*check_safe & BIT_CHECK_ERROR_CUR_TOO_LOW)){
				if(count_check_cur_too_low < MAX_NUM_DETECT) count_check_cur_too_low++;
				LOGD("current too low [%d]", count_check_cur_too_low);
				if(count_check_cur_too_low == MAX_NUM_DETECT){
					*check_safe |= BIT_CHECK_ERROR_CUR_TOO_LOW;
					return ERR_CUR_TOO_LOW;
				}			
			}
		}
	}
	return CODE_OK;
}

static err_code_t aptomat_check_error_power(uint32_t P_in, uint32_t P_threshold, uint8_t MAX_NUM_DETECT, uint8_t *check_safe)
{
	static s8 count_check_pow_err = 0;
	if(P_threshold == 0) return ERR_INVALID_ARG;
	if(P_in >= P_threshold){
		if(*check_safe & BIT_CHECK_ERROR_POWER){
			if(count_check_pow_err > 0) count_check_pow_err--;
			if(count_check_pow_err == 0){
				count_check_pow_err = 0;
				*check_safe &= ~BIT_CHECK_ERROR_POWER;
				LOGD("power err back to normal");
				return POWER_ERR_BACK_TO_NORMAL;
			}
		}
	}else{
		if(!(*check_safe & POWER_ERR_BACK_TO_NORMAL)){
			if(count_check_pow_err < MAX_NUM_DETECT) count_check_pow_err++;
			LOGD("power too high [%d]", count_check_pow_err);
			if(count_check_pow_err == MAX_NUM_DETECT){
				*check_safe |= POWER_ERR_BACK_TO_NORMAL;
				return ERR_POWER;
			}				
		}		
	}
	return CODE_OK;
}

/**********************************************************
					  AES SECURE
***********************************************************/

void task_check_kick_out(void)
{
#if EN_SECURE
	uint8_t flag_check_mess_secure = get_provision_secure();
	if (flag_check_mess_secure & BIT_CHECK_BIND_ALL && is_ota == false)
	{
		if (flag_check_mess_secure & BIT_CHECK_ENCRYPT_FAIL)
		{
			if (clock_time_s() < vrs_time_aes_err)
			{
				uint32_t time_temp = 0xffffffff - vrs_time_aes_err;
				if (clock_time_s() + time_temp > TIME_OUT_AES_FAIL_SECOND)
				{
					LOGW("time out 10s, kick out");
					kick_out(0);
				}
			}
			else
			{
				if (clock_time_s() - vrs_time_aes_err > TIME_OUT_AES_FAIL_SECOND)
				{
					LOGW("time out 10s, kick out");
					kick_out(0);
				}
			}
		}
		else if (!(flag_check_mess_secure & (BIT_CHECK_ENCRYPT_FAIL | BIT_CHECK_ENCRYPT_DONE)))
		{
			if (clock_time_s() < vrs_time_bind_all)
			{
				uint32_t time_temp = 0xffffffff - vrs_time_bind_all;
				if (clock_time_s() + time_temp > TIME_OUT_BIND_ALL_SECOND)
				{
					LOGW("time out 120s, kick out");
					kick_out(0);
				}
			}
			else
			{
				if (clock_time_s() - vrs_time_bind_all > TIME_OUT_BIND_ALL_SECOND)
				{
					LOGW("time out 120s, kick out");
					kick_out(0);
				}
			}
		}
	}
#endif
}

static void check_secure_event_handle(void *event, void *usr_data)
{
	secure_event_t event_id = *(secure_event_t *)event;
	switch (event_id)
	{
	case EVENT_SECURE_BIND_ALL:
	{
		if (flash_data.secure & BIT_CHECK_BIND_ALL)
			return;
		LOGD("bind all");
		flash_data.secure |= BIT_CHECK_BIND_ALL;
		vrs_time_bind_all = clock_time_s();
		// blink led
		led_mgmt_set_blink(LED_SIGNAL, 7, 300);
		break;
	}
	case EVENT_SECURE_ENCRYPT_DONE:
		flash_data.secure |= BIT_CHECK_ENCRYPT_DONE;
		break;
	case EVENT_SECURE_ENCRYPT_FAIL:
	{
		flash_data.secure |= BIT_CHECK_ENCRYPT_FAIL;
		vrs_time_aes_err = clock_time_s();
		break;
	}

	default:
		return;
		// break;
	}

	rd_write_flash_common();
}

void rd_write_flash_common(void)
{
	flash_erase_sector(RD_FLASH_ADDR_COMMON);
	flash_write_page(RD_FLASH_ADDR_COMMON, sizeof(flash_data_t), (unsigned char *)(&flash_data));
}

static void rd_init_flash_common_default(void)
{
	LOGW("init flash common default");
	flash_data.header[0] = FLASH_HEADER_1;
	flash_data.header[1] = FLASH_HEADER_2;
	flash_data.header[2] = FLASH_HEADER_1;
	flash_data.header[3] = FLASH_HEADER_2;
	flash_data.secure = 0;
	flash_data.I_threshold_low = I_THRESHOLD_LOW_DF;
	flash_data.I_threshold_high = I_THRESHOLD_HIGH_DF;
	flash_data.U_threshold_low = U_THRESHOLD_LOW_DF;
	flash_data.U_threshold_high = U_THRESHOLD_HIGH_DF;
	flash_data.P_threshold = P_THRESHOLD_DF;
	flash_data.TIME_CYCLE_READ_MS = TIME_CYCLE_READ_MS_DF;
	flash_data.MAX_CYCLE_DETECT_ERROR_I = MAX_CYCLE_DETECT_ERROR_I_DF;
	flash_data.MAX_CYCLE_DETECT_ERROR_P = MAX_CYCLE_DETECT_ERROR_P_DF;
	flash_data.MAX_CYCLE_DETECT_ERROR_U = MAX_CYCLE_DETECT_ERROR_U_DF;
	rd_write_flash_common();
}

void rd_dev_clear_flash_config(void)
{
	rd_init_flash_common_default();
}

void rd_init_flash_common(void)
{
	flash_read_page(RD_FLASH_ADDR_COMMON, sizeof(flash_data_t), (unsigned char *)(&flash_data));
	if (flash_data.header[0] != FLASH_HEADER_1 && flash_data.header[1] != FLASH_HEADER_2 &&
		flash_data.header[2] != FLASH_HEADER_1 && flash_data.header[3] != FLASH_HEADER_2)
	{
		rd_init_flash_common_default();
	}
	LOGI("Threshold: UH-%d, UL-%d, IH-%d, IL-%d, P-%d", flash_data.U_threshold_high, flash_data.U_threshold_low, flash_data.I_threshold_high, flash_data.I_threshold_low, flash_data.P_threshold);
	LOGI("MAX_NUM_DETECT U-I-P: %d-%d-%d", flash_data.MAX_CYCLE_DETECT_ERROR_U, flash_data.MAX_CYCLE_DETECT_ERROR_I, flash_data.MAX_CYCLE_DETECT_ERROR_P);
	LOGI("read param cycle (ms) %d\n", flash_data.TIME_CYCLE_READ_MS);
#if EN_SECURE
	if (get_provision_state() == STATE_DEV_PROVED)
	{
		uint8_t flag_check_mess_secure = get_provision_secure();
		if (!(flag_check_mess_secure & BIT_CHECK_ENCRYPT_DONE))
		{
			LOGW("check secure fail, kick out now!!");
			kick_out(0);
		}
	}
#endif
}
