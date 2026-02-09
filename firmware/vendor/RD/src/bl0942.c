/*
 * bl0942.c
 *
 *  Created on: Feb 9, 2026
 *      Author: PC5
 */

#include "tl_common.h"
#include "proj/mcu/watchdog_i.h"
#include "vendor/common/user_config.h"
#include "proj_lib/rf_drv.h"
#include "proj_lib/pm.h"
#include "proj_lib/ble/blt_config.h"
#include "proj_lib/ble/ll/ll.h"
#include "proj_lib/sig_mesh/app_mesh.h"

#include "../inc/bl0942.h"
#include "../inc/utils.h"



static uint8_t Set_CF_ZX[3] = {0x0E, 0x00, 0x00}; // 0x0E: 0000 1110: ZX 00, CF2 11, CF1 10
static uint8_t Set_Gain[3] = {0x03, 0x00, 0x00};
static uint8_t Set_Soft_Reset[3] = {0x5a, 0x5a, 0x5a};

static uint8_t reg_read = BL0942_REG_NONE;

static inline void uart_send_data(uint8_t *data, uint8_t len){
	while(*data != '\0'){
		uart_send_byte(*(data++));
	}
}

static inline uint8_t get_register_is_reading(void){
    return reg_read;
}

static inline void clear_register_is_reading(void){
	reg_read = BL0942_REG_NONE;
}

err_code_t bl0942_send_unlock(void){
    uint8_t data_unlock[6] = {0};
    uint32_t CRC_check_temp = BL0942_CMD_WRITE + BL0942_REG_UNLOCK + 0x55;
    uint8_t CRC_check = ~(CRC_check_temp & 0xff);
    data_unlock[0] = BL0942_CMD_WRITE;
    data_unlock[1] = BL0942_REG_UNLOCK;
    data_unlock[2] = 0x55;
    data_unlock[3] = 0x00;
    data_unlock[4] = 0x00;//0x55;
    data_unlock[5] = CRC_check;

    uart_send_data(data_unlock, 6);
    LOGI("Unlock BL0942\t");
    return CODE_OK;
}

err_code_t bl0942_send_setup(uint8_t REG, uint8_t *data){
	bl0942_send_unlock();
    uint8_t data_send[6] = {0};
    uint16_t CRC_Temp = BL0942_CMD_WRITE + REG + data[0] + data[1] + data[2];
    uint8_t CRC_Check = ~(CRC_Temp & 0xff);
    data_send[0] = BL0942_CMD_WRITE;
    data_send[1] = REG;
    data_send[2] = data[0];
    data_send[3] = data[1];
    data_send[4] = data[2];
    data_send[5] = CRC_Check;

    uart_send_data(data_send, 6);
    LOGI("send: %02x %02x %02x %02x %02x %02x\n", data_send[0], data_send[1], data_send[2], data_send[3], data_send[4], data_send[5]);
    return CODE_OK;
}

err_code_t bl0942_send_read_cmd(uint8_t REG){
    uint8_t tx_data[6] = {0};
    uint16_t CRC_Temp = BL0942_CMD_READ + REG;
    uint8_t CRC_Check = ~(CRC_Temp & 0xff);
    reg_read = REG;

    tx_data[0] = BL0942_CMD_READ;
    tx_data[1] = REG;
    tx_data[2] = 0x00;
    tx_data[3] = 0x00;
    tx_data[4] = 0x00;
    tx_data[5] = CRC_Check;

    LOGI("READ REG: %02x", REG);
    uart_send_data(tx_data, 6);
    return CODE_OK;
}

static s16 bl0942_get_raw_data(uint8_t *raw_data, uint16_t len){
	uint16_t time_out = 0;
	uint16_t len_real = 0;

	len_real = rd_buffer_get_size();
	while(len_real < len){
		time_out++;
		if(time_out > 500) // > 100ms
		{
			LOGE("time out receive raw data");
			rd_flush();
			return -1;
		}
		sleep_ms(10); wd_clear();
		len_real += rd_buffer_get_size();
	}
	rd_buffer_get_data(raw_data, len_real);
	rd_flush();

	LOGD("raw data:");
	LOG_HEX_BUFF(raw_data, len_real);
	return len_real;
}

static err_code_t bl0942_get_value(uint8_t value[3], uint8_t *raw_data){
	uint8_t reg_temp = get_register_is_reading();
    if(reg_temp == BL0942_REG_NONE){
        return CODE_ERR;
    }
    uint16_t CRC_Temp = BL0942_CMD_READ + reg_temp + raw_data[0] + raw_data[1] + raw_data[2] ;
    uint8_t CRC_Check = ~(CRC_Temp & 0xff);
    if(CRC_Check != raw_data[3]){
    	LOGE("check crc fail, reg: %02x", reg_temp);
    	clear_register_is_reading();
        return CODE_ERR;
    }
    value[0] = raw_data[0];
    value[1] = raw_data[1];
    value[2] = raw_data[2];
    clear_register_is_reading();
    return CODE_OK;
}

uint32_t bl0942_read_data_unsigned(uint8_t REG){
    uint8_t reg = 0;
    uint8_t data[3] = {0};
    uint8_t raw[8];

    bl0942_send_read_cmd(REG);
    if(bl0942_get_raw_data(raw, 4) == -1){
        LOGE("read raw data BL0942 FAIL > _ >");
        return 0;
    }
    if(bl0942_get_value(data, raw) != CODE_OK){
        LOGE("check data BL0942 FAIL ^_^");
        return 0;
    }
    uint32_t value = ((uint32_t)data[2] << 16) | ((uint32_t)data[1] << 8) | data[0];
    LOGI("[BL0942] REG : %02X, data: %02x %02x %02x, value (unsigned): %ld", reg, data[0], data[1], data[2], value);
    return value;
}

s32 bl0942_read_data_signed(uint8_t REG){
    uint8_t reg = 0;
    uint8_t data[3] = {0};
    uint8_t raw[8];

    bl0942_send_read_cmd(REG);
    if(bl0942_get_raw_data(raw, 4) == -1){
        LOGE("read raw data BL0942 FAIL > _ >");
        return 0;
    }
    if(bl0942_get_value(data, raw) != CODE_OK){
        LOGE("check data BL0942 FAIL ^_^");
        return 0;
    }

    uint32_t value = ((uint32_t)data[2] << 16) | ((uint32_t)data[1] << 8) | data[0];
    if(value & 0x800000){
        value |= 0xFF000000;
    }
    LOGI("[BL0942] REG : %02X, data: %02x %02x %02x, value (signed): %ld\n", reg, data[0], data[1], data[2], value);
    return value;
}

err_code_t bl0942_init(void){
	static bool isBL0942Init = false;
    if(isBL0942Init == false){
    	bl0942_send_setup(BL0942_REG_SOFT_RESET, Set_Soft_Reset);
    	bl0942_send_setup(BL0942_REG_OT_FUNX, Set_CF_ZX);
    	bl0942_send_setup(BL0942_REG_GAIN_CR, Set_Gain);

        sleep_ms(500); wd_clear();
        sleep_ms(500); wd_clear();

        uint8_t gain_cr_read = (uint8_t)bl0942_read_data_unsigned(BL0942_REG_GAIN_CR);
        uint8_t ot_funx_read = (uint8_t)bl0942_read_data_unsigned(BL0942_REG_OT_FUNX);

        printf("gain_cr_read: %02x, ot_funx_read: %02x\n", gain_cr_read, ot_funx_read);
        if(gain_cr_read == Set_Gain[0] && ot_funx_read == Set_CF_ZX[0]){
        	isBL0942Init = true;
            LOGI("[BL0942] setup successfully");
            return CODE_OK;
        }else{
        	isBL0942Init = false;
            LOGE("[BL0942] Failed to setup BL0942");
            return CODE_ERR;
        }
    }
    return CODE_ERR;
}
