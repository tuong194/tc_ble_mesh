/*
 * mess_handle.c
 *
 *  Created on: Feb 12, 2026
 *      Author: PC5
 */

#include "../inc/mess_handle.h"
//#include "../inc/utils.h"
#include "../inc/rd_err.h"
#include "../inc/controller.h"
#include "../inc/Define.h"
#include "../inc/rd_flash.h"

static uint16_t GATEWAY_ADDR = 0x0001;
static event_post_cb_t secure_cb;

/********************** OPCODE E0 ***********************/
static int rd_handle_save_gw(uint8_t *par, uint8_t src_adr);
static int rd_handle_check_secure_and_get_type(uint8_t *par, uint8_t src_adr);

/********************** OPCODE E2 ***********************/
static int rd_handle_get_param(uint8_t *par);
static int rd_handle_set_threshold_power(uint8_t *par);
static int rd_handle_set_threshold_current(uint8_t *par);
static int rd_handle_set_countdown(uint8_t *par);
static int rd_handle_set_time_and_num_detect(uint8_t *par);

uint16_t rd_get_gateway_addr(void){
	return GATEWAY_ADDR;
}

void init_flash_gateway(void){
	uint8_t gw_buff[4];
	flash_read_page(RD_FLASH_ADDR_GATEWAY, 4, (unsigned char *)gw_buff);
//	GATEWAY_ADDR = ((gw_buff[1] << 8) & 0xff) | gw_buff[0];
	if(gw_buff[0] != 0x55 && gw_buff[1] != 0xAA){
		gw_buff[0] = 0x55; gw_buff[1] = 0xAA;
		GATEWAY_ADDR = 0x0001;
	    gw_buff[2] = GATEWAY_ADDR & 0xff;
	    gw_buff[3] = (GATEWAY_ADDR >> 8) & 0xff;
	}else{
		GATEWAY_ADDR = ((gw_buff[3] << 8) & 0xff) | gw_buff[2];
	}
	LOGI("GATEWAY addr: %04x", GATEWAY_ADDR);
}

void rd_register_event_secure(event_post_cb_t cb){
    if(cb) secure_cb = cb;
}

int RD_mess_handle_opcode_E0(u8 *par, int par_len, mesh_cb_fun_par_t *cb_par)
{
    uint16_t header = par[1] << 8 | par[0];
    switch (header)
    {
    case RD_HEADER_SAVE_GATEWAY:
        LOGD("[E0] save gw");
        rd_handle_save_gw(par, cb_par->adr_src);
        break;
    case RD_HEADER_AES_AND_GET_TYPE:
        LOGD("[E0] check secure");
        rd_handle_check_secure_and_get_type(par, cb_par->adr_dst);
        break;
    default:
        LOGE("unknown header %04x, opcode E0", header);
        break;
    }
    return 0;
}

int RD_mess_handle_opcode_E2(u8 *par, int par_len, mesh_cb_fun_par_t *cb_par)
{
    int ret = 0;
//    LOGI("opcode E2: src addr: 0x%04x, dst_addr: 0x%04x", cb_par->adr_src, cb_par->adr_dst);
//    LOG_HEX_BUFF(par, 6);
    uint16_t header = par[1] << 8 | par[0];
    switch (header)
    {
    case RD_HEADER_GET_PARAM:
    	ret = rd_handle_get_param(par);
    	break;
    case RD_HEADER_SET_THRESHOLD_CURRENT:
        ret = rd_handle_set_threshold_current(par);
        break;
    case RD_HEADER_SET_THRESHOLD_POWER:
        ret = rd_handle_set_threshold_power(par);
        break;
    case RD_HEADER_COUNTDOWN:
        ret = rd_handle_set_countdown(par);
        break;
    case RD_HEADER_SET_TIME_AND_NUM_DETECT:
    	ret = rd_handle_set_time_and_num_detect(par);
    	break;

    default:
        LOGE("unknown header %04x, opcode E2", header);
        break;
    }
    return ret;
}

static int rd_handle_save_gw(uint8_t *par, uint8_t src_adr)
{
    uint8_t rsp_buf[8];
    if (par[2] || par[3])
    {
        GATEWAY_ADDR = par[3] << 8 | par[2];
    }
    else
    {
        GATEWAY_ADDR = src_adr;
    }

    uint8_t gw_buff[4] = {0x55, 0xAA,0,0};
    gw_buff[2] = GATEWAY_ADDR & 0xff;
    gw_buff[3] = (GATEWAY_ADDR >> 8) & 0xff;

	flash_erase_sector(RD_FLASH_ADDR_GATEWAY);
	flash_write_page(RD_FLASH_ADDR_GATEWAY, 4, (unsigned char *)gw_buff);

    rsp_buf[0] = RD_HEADER_SAVE_GATEWAY & 0xff;
    rsp_buf[1] = (RD_HEADER_SAVE_GATEWAY >> 8) & 0xff;
    rsp_buf[2] = GATEWAY_ADDR & 0xff;
    rsp_buf[3] = (GATEWAY_ADDR >> 8) & 0xff;
    rsp_buf[4] = PROVIDER_MAIN;
    rsp_buf[5] = PROVIDER_SUB;
    rsp_buf[6] = 0;
    rsp_buf[7] = 0;
    return mesh_tx_cmd2normal_primary(RD_OPCODE_RSP_SCAN_DEV, rsp_buf, 8, GATEWAY_ADDR, 0);
}

static secure_event_t event_secure = EVENT_SECURE_MAX;

void rd_mess_post_event_bind_all(void){
    event_secure = EVENT_SECURE_BIND_ALL;
    if(secure_cb) secure_cb(&event_secure, NULL);    
}
static int rd_handle_check_secure_and_get_type(uint8_t *par, uint8_t src_adr)
{
    if (is_provision_success()) // get_provision_state() == STATE_DEV_PROVED
    {
        uint8_t rsp_buf[8];
        if (rd_aesRecheck(src_adr, &par[2]))
        {
            LOGD("encrypt DONE !!!");
            rsp_buf[0] = RD_HEADER_AES_AND_GET_TYPE & 0xff;
            rsp_buf[1] = (RD_HEADER_AES_AND_GET_TYPE >> 8) & 0xff;
            rsp_buf[2] = MAINTYPE;
            rsp_buf[3] = FEATURE;
            rsp_buf[4] = NAME;
            rsp_buf[5] = 0x00;
            rsp_buf[6] = VERSION_MAIN;
            rsp_buf[7] = VERSION_SUB;

            event_secure = EVENT_SECURE_ENCRYPT_DONE;
            if(secure_cb) secure_cb(&event_secure, NULL);
        }
        else
        {
            LOGD("encrypt FAIL !!!");
            rsp_buf[0] = RD_HEADER_AES_AND_GET_TYPE & 0xff;
            rsp_buf[1] = (RD_HEADER_AES_AND_GET_TYPE >> 8) & 0xff;
            rsp_buf[2] = 0xff;
            rsp_buf[3] = 0xfe;
            rsp_buf[4] = 0xff;
            rsp_buf[5] = 0xfe;
            rsp_buf[6] = 0xff;
            rsp_buf[7] = 0xfe;

            event_secure = EVENT_SECURE_ENCRYPT_FAIL;
            if(secure_cb) secure_cb(&event_secure, NULL);
        }
        return mesh_tx_cmd2normal_primary(RD_OPCODE_RSP_SCAN_DEV, rsp_buf, 8, GATEWAY_ADDR, 0);
    }
    return -1;
}

static int rd_handle_set_threshold_power(uint8_t *par)
{
    uint8_t rsp_buf[8];
    uint32_t threshold = ((par[2] << 8) | par[3]) * 100 + ((par[4] << 8) | par[5]);
    LOGI("set threshold power: %u/100 W", threshold);
    dev_set_threshold_power(threshold);

    rsp_buf[0] = RD_HEADER_SET_THRESHOLD_POWER & 0xff;
    rsp_buf[1] = (RD_HEADER_SET_THRESHOLD_POWER >> 8) & 0xff;
    rsp_buf[2] = par[2];
    rsp_buf[3] = par[3];
    rsp_buf[4] = par[4];
    rsp_buf[5] = par[5];

    return mesh_tx_cmd2normal_primary(RD_OPCODE_RSP_PRODUCT_FEATURE, rsp_buf, 8, GATEWAY_ADDR, 0);
}

static int rd_handle_set_threshold_current(uint8_t *par)
{
    uint8_t rsp_buf[8];
    uint32_t threshold = ((par[2] << 8) | par[3]) * 100 + ((par[4] << 8) | par[5]);
    LOGI("set threshold current: %u/100 A", threshold);
    dev_set_threshold_current(threshold);

    rsp_buf[0] = RD_HEADER_SET_THRESHOLD_CURRENT & 0xff;
    rsp_buf[1] = (RD_HEADER_SET_THRESHOLD_CURRENT >> 8) & 0xff;
    rsp_buf[2] = par[2];
    rsp_buf[3] = par[3];
    rsp_buf[4] = par[4];
    rsp_buf[5] = par[5];

    return mesh_tx_cmd2normal_primary(RD_OPCODE_RSP_PRODUCT_FEATURE, rsp_buf, 8, GATEWAY_ADDR, 0);
}

static int rd_handle_set_countdown(uint8_t *par)
{
    uint8_t rsp_buf[8];
    uint16_t time_countdown = par[4] << 8 | par[3];//*(uint16_t *)(&par[2]);
    LOGD("set time countdown: %u s", time_countdown);
    memcpy(rsp_buf, par, 4);
    return mesh_tx_cmd2normal_primary(RD_OPCODE_RSP_PRODUCT_FEATURE, rsp_buf, 8, GATEWAY_ADDR, 0);
}

static int rd_handle_get_param(uint8_t *par){
	LOGD("get electrical param");
	dev_rsp_param_to_gw(TYPE_GET_VOLTAGE);
	sleep_ms(800); wd_clear();
	dev_rsp_param_to_gw(TYPE_GET_CURRENT);
	sleep_ms(800); wd_clear();
	dev_rsp_param_to_gw(TYPE_GET_POWER);
	sleep_ms(800); wd_clear();
	dev_rsp_param_to_gw(TYPE_GET_POWER_CONSUME);
	return 0;
}

static int rd_handle_set_time_and_num_detect(uint8_t *par){
	LOGD("set time cycle read param and detect error");
	uint8_t rsp_buf[8];
	uint32_t time_ms = (par[2] << 24) | (par[3] << 16) | (par[4] << 8) | par[5];
	LOGD("set time cycle: %u ms, num: detect_I: %d, detect_P: %d", time_ms, par[6], par[7]);
	dev_set_time_cycle_read_param_electrical(time_ms);
	dev_set_max_num_detect_err_current(par[6]);
	dev_set_max_num_detect_err_power(par[7]);
	memcpy(rsp_buf, par, 8);
	return mesh_tx_cmd2normal_primary(RD_OPCODE_RSP_PRODUCT_FEATURE, rsp_buf, 8, GATEWAY_ADDR, 0);
}


int dev_rsp_param_to_gw(type_get_para type)
{
    uint8_t rsp_buf[8];
    uint32_t value = 0;
    switch (type)
    {
    case TYPE_GET_VOLTAGE:
        value = aptomat_get_voltage();
        break;
    case TYPE_GET_CURRENT:
        value = aptomat_get_current();
        break;
    case TYPE_GET_POWER:
        value = aptomat_get_power();
        break;
    case TYPE_GET_POWER_CONSUME:
        value = aptomat_get_power_consume();
        break;

    default:
        break;
    }

    rsp_buf[0] = 0x17;
    rsp_buf[1] = (uint8_t)type;
    rsp_buf[2] = (value >> 24) & 0xff;
    rsp_buf[3] = (value >> 16) & 0xff;
    rsp_buf[4] = (value >> 8) & 0xff;
    rsp_buf[5] = value & 0xff;

    return mesh_tx_cmd2normal_primary(RD_OPCODE_REPORT_PARAM_ELECTRICAL, rsp_buf, 6, GATEWAY_ADDR, 0);
}
