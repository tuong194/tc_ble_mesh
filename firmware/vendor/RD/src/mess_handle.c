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

static uint16_t GATEWAY_ADDR = 0x0001;
static int rd_handle_set_threshold_power(uint8_t* par);
static int rd_handle_set_threshold_current(uint8_t* par);
static int rd_handle_set_countdown(uint8_t* par);
uint16_t rd_handle_mess_save_gw(uint8_t* para, uint16_t srcAddr);

int RD_mess_handle_opcode_E0(u8* par, int par_len, mesh_cb_fun_par_t* cb_par)
{
    uint16_t header = *(uint16_t*)par;
    switch (header)
    {
    case RD_HEADER_SAVE_GATEWAY:
        LOGD("[E0] save gw");
        GATEWAY_ADDR = rd_handle_mess_save_gw(&par[2], cb_par->adr_src);
        //RD_Flash_Save_GW(GATEWAY_ADDR);

        uint8_t rsp_buf[8];
        rsp_buf[0] = RD_HEADER_SAVE_GATEWAY & 0xff;
        rsp_buf[1] = (RD_HEADER_SAVE_GATEWAY >> 8) & 0xff;
        rsp_buf[2] = GATEWAY_ADDR & 0xff;
        rsp_buf[3] = (GATEWAY_ADDR >> 8) & 0xff;
        // rsp_buf[4] = PROVIDER_MAIN;
        // rsp_buf[5] = PROVIDER_SUB;
        rsp_buf[6] = 0;
        rsp_buf[7] = 0;
        mesh_tx_cmd2normal_primary(cb_par->op_rsp, rsp_buf, 8, cb_par->adr_src, 2);
        break;
    case RD_HEADER_AES_AND_GET_TYPE:
        LOGD("[E0] check secure");
        if (is_provision_success())  //get_provision_state() == STATE_DEV_PROVED
        {
            uint8_t rsp_buf[8];
            if (rd_aesRecheck(cb_par->adr_dst, &par[2]))
            {
                LOGD("encrypt DONE !");
                rsp_buf[0] = RD_HEADER_AES_AND_GET_TYPE & 0xff;
                rsp_buf[1] = (RD_HEADER_AES_AND_GET_TYPE >> 8) & 0xff;
                // rsp_buf[2] = MAINTYPE;
                // rsp_buf[3] = FEATURE;
                // rsp_buf[4] = NAME;
                rsp_buf[5] = 0x00;
                // rsp_buf[6] = VERSION_MAIN;
                // rsp_buf[7] = VERSION_SUB;
                mesh_tx_cmd2normal_primary(cb_par->op_rsp, rsp_buf, 8, cb_par->adr_src, 2);
            }
            else {
                LOGD("encrypt FAIL !!!");
                rsp_buf[0] = RD_HEADER_AES_AND_GET_TYPE & 0xff;
                rsp_buf[1] = (RD_HEADER_AES_AND_GET_TYPE >> 8) & 0xff;
                rsp_buf[2] = 0xff;
                rsp_buf[3] = 0xfe;
                rsp_buf[4] = 0xff;
                rsp_buf[5] = 0xfe;
                rsp_buf[6] = 0xff;
                rsp_buf[7] = 0xfe;
                mesh_tx_cmd2normal_primary(cb_par->op_rsp, rsp_buf, 8, cb_par->adr_src, 2);
            }
        }
        break;
    default:
        LOGE("unknown header %04x, opcode E0", header);
        break;
    }
    return 0;
}

int RD_mess_handle_opcode_E2(u8* par, int par_len, mesh_cb_fun_par_t* cb_par)
{
    int ret = 0;
    uint16_t header = *(uint16_t*)par;
    switch (header)
    {
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

static int rd_handle_set_threshold_power(uint8_t* par) {
    uint8_t rsp_buf[8];
    uint32_t threshold = (par[2] << 24) | (par[3] << 16) | (par[4] << 8) | par[5];
    dev_set_threshold_power(threshold);

    rsp_buf[0] = RD_HEADER_SET_THRESHOLD_POWER & 0xff;
    rsp_buf[1] = (RD_HEADER_SET_THRESHOLD_POWER >> 8) & 0xff;
    rsp_buf[2] = par[2];
    rsp_buf[3] = par[3];
    rsp_buf[4] = par[4];
    rsp_buf[5] = par[5];

    return mesh_tx_cmd2normal_primary(RD_OPCODE_RSP_PRODUCT_FEATURE, rsp_buf, 8, GATEWAY_ADDR, 2);

}

static int rd_handle_set_threshold_current(uint8_t* par) {
    uint8_t rsp_buf[8];
    uint32_t threshold = (par[2] << 24) | (par[3] << 16) | (par[4] << 8) | par[5];
    dev_set_threshold_current(threshold);

    rsp_buf[0] = RD_HEADER_SET_THRESHOLD_CURRENT & 0xff;
    rsp_buf[1] = (RD_HEADER_SET_THRESHOLD_CURRENT >> 8) & 0xff;
    rsp_buf[2] = par[2];
    rsp_buf[3] = par[3];
    rsp_buf[4] = par[4];
    rsp_buf[5] = par[5];

    return mesh_tx_cmd2normal_primary(RD_OPCODE_RSP_PRODUCT_FEATURE, rsp_buf, 8, GATEWAY_ADDR, 2);
}

static int rd_handle_set_countdown(uint8_t* par) {
    uint8_t rsp_buf[8];
    uint16_t time_countdown = *(uint16_t*)(&par[2]);
    LOGD("set time countdown: %d s", time_countdown);
    memcpy(rsp_buf, par, 4);
    return mesh_tx_cmd2normal_primary(RD_OPCODE_RSP_PRODUCT_FEATURE, rsp_buf, 8, GATEWAY_ADDR, 2);
}

uint16_t rd_handle_mess_save_gw(uint8_t* para, uint16_t srcAddr) {
    uint16_t GW_Addr = 0x0001;
    if (para[0] || para[1]) {
        GW_Addr = para[1] << 8 | para[0];
    }
    else {
        GW_Addr = srcAddr;
    }
    return GW_Addr;
}

int dev_rsp_param_to_gw(type_get_para type) {
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

    return mesh_tx_cmd2normal_primary(RD_OPCODE_RSP_PRODUCT_FEATURE, rsp_buf, 8, GATEWAY_ADDR, 2);
}

