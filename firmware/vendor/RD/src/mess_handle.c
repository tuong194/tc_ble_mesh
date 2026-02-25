/*
 * mess_handle.c
 *
 *  Created on: Feb 12, 2026
 *      Author: PC5
 */

#include "../inc/mess_handle.h"
#include "../inc/utils.h"
#include "../inc/rd_err.h"
#include "../inc/controller.h"
#include "../inc/Define.h"

static uint16_t GATEWAY_ADDR = 0x0001;

/********************** OPCODE E0 ***********************/
static int rd_handle_save_gw(uint8_t *par, uint8_t src_adr);
static int rd_handle_check_secure_and_get_type(uint8_t *par, uint8_t src_adr);

/********************** OPCODE E2 ***********************/
static int rd_handle_get_param(uint8_t *par);
static int rd_handle_set_threshold_power(uint8_t *par);
static int rd_handle_set_threshold_current(uint8_t *par);
static int rd_handle_set_countdown(uint8_t *par);


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
        rd_handle_check_secure_and_get_type(par, cb_par->adr_src);
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
//    LOG_HEX_BUFF(par, 6);
    uint16_t header = par[1] << 8 | par[0];
    switch (header)
    {
    case RD_HEADER_GET_PARAM:
    	ret = rd_handle_get_param(par);
    case RD_HEADER_SET_THRESHOLD_CURRENT:
        ret = rd_handle_set_threshold_power(par);
        break;
    case RD_HEADER_SET_THRESHOLD_POWER:
        ret = rd_handle_set_threshold_current(par);
        break;
    case RD_HEADER_COUNTDOWN:
        ret = rd_handle_set_countdown(par);
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
    if (par[0] || par[1])
    {
        GATEWAY_ADDR = par[1] << 8 | par[0];
    }
    else
    {
        GATEWAY_ADDR = src_adr;
    }
    // RD_Flash_Save_GW(GATEWAY_ADDR);

    rsp_buf[0] = RD_HEADER_SAVE_GATEWAY & 0xff;
    rsp_buf[1] = (RD_HEADER_SAVE_GATEWAY >> 8) & 0xff;
    rsp_buf[2] = GATEWAY_ADDR & 0xff;
    rsp_buf[3] = (GATEWAY_ADDR >> 8) & 0xff;
    rsp_buf[4] = PROVIDER_MAIN;
    rsp_buf[5] = PROVIDER_SUB;
    rsp_buf[6] = 0;
    rsp_buf[7] = 0;
    return mesh_tx_cmd2normal_primary(RD_OPCODE_RSP_SCAN_DEV, rsp_buf, 8, src_adr, 0);
}

static int rd_handle_check_secure_and_get_type(uint8_t *par, uint8_t src_adr)
{
    if (is_provision_success()) // get_provision_state() == STATE_DEV_PROVED
    {
        uint8_t rsp_buf[8];
        if (rd_aesRecheck(src_adr, &par[2]))
        {
            LOGD("encrypt DONE !");
            rsp_buf[0] = RD_HEADER_AES_AND_GET_TYPE & 0xff;
            rsp_buf[1] = (RD_HEADER_AES_AND_GET_TYPE >> 8) & 0xff;
            rsp_buf[2] = MAINTYPE;
            rsp_buf[3] = FEATURE;
            rsp_buf[4] = NAME;
            rsp_buf[5] = 0x00;
            rsp_buf[6] = VERSION_MAIN;
            rsp_buf[7] = VERSION_SUB;
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
        }
        return mesh_tx_cmd2normal_primary(RD_OPCODE_RSP_SCAN_DEV, rsp_buf, 8, src_adr, 0);
    }
    return -1;
}

static int rd_handle_set_threshold_power(uint8_t *par)
{
    uint8_t rsp_buf[8];
    uint32_t threshold = ((par[2] << 8) | par[3]) * 100 + ((par[4] << 8) | par[5]);
    LOGI("set threshold power: %u W", threshold);
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
    LOGI("set threshold curent: %u A", threshold);
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
    uint16_t integer_part = value / 100;
    uint16_t decimal_part = value % 100;
    rsp_buf[0] = RD_HEADER_GET_PARAM & 0xff;
    rsp_buf[1] = (RD_HEADER_GET_PARAM >> 8) & 0xff;
    rsp_buf[2] = (uint8_t)type;
    rsp_buf[3] = (integer_part >> 8) & 0xff;
    rsp_buf[4] = integer_part & 0xff;
    rsp_buf[5] = (decimal_part >> 8) & 0xff;
    rsp_buf[6] = decimal_part & 0xff;

    return mesh_tx_cmd2normal_primary(RD_OPCODE_RSP_PRODUCT_FEATURE, rsp_buf, 8, GATEWAY_ADDR, 0);
}
