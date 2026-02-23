/*
 * k9b.c
 *
 *  Created on: Feb 12, 2026
 *      Author: PC5
 */


 #include "k9b.h"
 #include "utils.h"

 #define NUM_ELEMENT 1
 #define FLASH_HEADER_1 0x55
 #define FLASH_HEADER_2 0xAA

 typedef struct
{
	uint8_t Pair_KP9OnOff_Flag;
	uint8_t Button_ID_OnOff;
    uint32_t Clock_time_start_pair_onoff;
} Sw_Working_Stt_Str;

struct button_value_onoff{
	uint32_t last_counter_onoff;
	uint32_t time_tongle_onoff; 
};

typedef struct
{
	uint8_t pos_save_onoff_next;
  	uint32_t MacK9B[MAX_NUM_K9ONOFF];
  	uint8_t K9B_BtKey[MAX_NUM_K9ONOFF];
}k9b_para_onoff_t; 

typedef struct
{
	uint8_t Header[4];
	k9b_para_onoff_t K9B_onoff[4];
}flash_K9B_onoff_t;

static k9b_proxy_t *switchKP9_proxy;
static Sw_Working_Stt_Str Sw_Working_Stt_Val;

flash_K9B_onoff_t flash_K9B_onoff;

static void k9b_init_flash_onoff_df(void)
{
	flash_K9B_onoff.Header[0] = FLASH_HEADER_1;
	flash_K9B_onoff.Header[1] = FLASH_HEADER_2;
	flash_K9B_onoff.Header[2] = FLASH_HEADER_1;
	flash_K9B_onoff.Header[3] = FLASH_HEADER_2;
	for (size_t i = 0; i < NUM_ELEMENT; i++)
	{
		flash_K9B_onoff.K9B_onoff[i].pos_save_onoff_next = 0;
		for (size_t j = 0; j < MAX_NUM_K9ONOFF; j++)
		{
			flash_K9B_onoff.K9B_onoff[i].MacK9B[j] = 0;
			flash_K9B_onoff.K9B_onoff[i].K9B_BtKey[j] = 0;
		}
	}
	k9b_save_flash_onoff();
}

static void k9b_init_flash_onoff(void)
{
	//rd_read_flash(KEY_FLASH_K9B_ONOFF, &flash_K9B_onoff.Header[0], sizeof(flash_K9B_onoff)); //RD_NOTE edit read flash k9b 

	if (flash_K9B_onoff.Header[0] != FLASH_HEADER_1 && flash_K9B_onoff.Header[1] != FLASH_HEADER_2 &&
		flash_K9B_onoff.Header[3] != FLASH_HEADER_1 && flash_K9B_onoff.Header[4] != FLASH_HEADER_2)
	{
		k9b_init_flash_onoff_df();
	}
	ESP_LOGI("K9B MANAGER", "init flash K9B onoff");
	for (size_t i = 0; i < NUM_ELEMENT; i++)
	{
		printf("K9B: button index %d\n", i);
		printf("pos_save_onoff_next: %d\n", flash_K9B_onoff.K9B_onoff[i].pos_save_onoff_next);
		for (size_t j = 0; j < flash_K9B_onoff.K9B_onoff[i].pos_save_onoff_next; j++)
		{
			if (flash_K9B_onoff.K9B_onoff[i].MacK9B[j] != 0)
			{
				printf("MAC: mac K9B %lu\n", flash_K9B_onoff.K9B_onoff[i].MacK9B[j]);
			}
		}
		printf("------------------------------------------\n");
	}
}

void k9b_init_flash(void)
{
	k9b_init_flash_onoff();
}

void k9b_save_flash_onoff(void)
{
	//rd_write_flash(KEY_FLASH_K9B_ONOFF, &flash_K9B_onoff.Header[0], sizeof(flash_K9B_onoff)); //RD_NOTE edit read flash k9b 
}


uint8_t k9b_get_state_pair_onoff(void)
{
	return Sw_Working_Stt_Val.Pair_KP9OnOff_Flag;
}

void k9b_set_start_pair_onoff(uint8_t ele)
{
	ESP_LOGI("K9B MANAGER", "start pair onoff btn index: %d", ele);
	Sw_Working_Stt_Val.Clock_time_start_pair_onoff = esp_timer_get_time();
	Sw_Working_Stt_Val.Button_ID_OnOff = ele;
	Sw_Working_Stt_Val.Pair_KP9OnOff_Flag = 0x01;
}


void k9b_clear_pair_onoff(void)
{
	Sw_Working_Stt_Val.Button_ID_OnOff = 0xff;
	Sw_Working_Stt_Val.Pair_KP9OnOff_Flag = 0;
}


static void k9b_scan_timeout_onoff(void)
{
	if (Sw_Working_Stt_Val.Pair_KP9OnOff_Flag == 1 && (esp_timer_get_time() - Sw_Working_Stt_Val.Clock_time_start_pair_onoff >= TIME_OUT_SCAN_KP9))
	{
		ESP_LOGE("KP9 MANAGER", "time out scan onoff");
		k9b_clear_pair_onoff();
	}
}

void k9b_loop_check_pair_time_out(void)
{
	k9b_scan_timeout_onoff();
}

/*----------------------------onoff----------------------------------*/
s8 check_mac_onoff_save_yet(uint8_t index, uint32_t mac)
{
	s8 pos = -1;
	for (size_t i = 0; i < MAX_NUM_K9ONOFF; i++)
	{
		if (flash_K9B_onoff.K9B_onoff[index].MacK9B[i] == mac)
		{
			pos = i;
		}
	}
	return pos;
}

void save_data_onoff_one_btn(uint8_t index, uint32_t mac, uint8_t key)
{
	uint8_t pos_save_next = flash_K9B_onoff.K9B_onoff[index].pos_save_onoff_next;

	if (pos_save_next < MAX_NUM_K9ONOFF)
	{
		flash_K9B_onoff.K9B_onoff[index].MacK9B[pos_save_next] = mac;
		flash_K9B_onoff.K9B_onoff[index].K9B_BtKey[pos_save_next] = key;
		flash_K9B_onoff.K9B_onoff[index].pos_save_onoff_next++;
	}
	else
	{
		for (size_t i = 0; i < MAX_NUM_K9ONOFF - 1; i++)
		{
			flash_K9B_onoff.K9B_onoff[index].MacK9B[i] = flash_K9B_onoff.K9B_onoff[index].MacK9B[i + 1];
			flash_K9B_onoff.K9B_onoff[index].K9B_BtKey[i] = flash_K9B_onoff.K9B_onoff[index].K9B_BtKey[i + 1];
		}
		flash_K9B_onoff.K9B_onoff[index].MacK9B[MAX_NUM_K9ONOFF - 1] = mac;
		flash_K9B_onoff.K9B_onoff[index].K9B_BtKey[MAX_NUM_K9ONOFF - 1] = key;
	}
	k9b_save_flash_onoff();
}

void k9b_delete_onoff(uint8_t index, uint8_t pos)
{
	uint8_t pos_next = flash_K9B_onoff.K9B_onoff[index].pos_save_onoff_next;
	for (size_t i = pos; i < pos_next; i++)
	{
		flash_K9B_onoff.K9B_onoff[index].MacK9B[i] = flash_K9B_onoff.K9B_onoff[index].MacK9B[i + 1];
		flash_K9B_onoff.K9B_onoff[index].K9B_BtKey[i] = flash_K9B_onoff.K9B_onoff[index].K9B_BtKey[i + 1];
	}
	flash_K9B_onoff.K9B_onoff[index].pos_save_onoff_next--;
	pos_next--;
	flash_K9B_onoff.K9B_onoff[index].MacK9B[pos_next] = 0;
	flash_K9B_onoff.K9B_onoff[index].K9B_BtKey[pos_next] = 0;
	k9b_save_flash_onoff();
}

void K9B_delete_all_onoff_one_btn(uint8_t index)
{
	ESP_LOGE("KP9 MANAGER", "delete all K9B index %d", index);
	if(Sw_Working_Stt_Val.Pair_KP9OnOff_Flag == 0x01){
		k9b_clear_pair_onoff();
		flash_K9B_onoff.K9B_onoff[index].pos_save_onoff_next = 0;
		for (size_t i = 0; i < MAX_NUM_K9ONOFF; i++)
		{
			flash_K9B_onoff.K9B_onoff[index].MacK9B[i] = 0;
			flash_K9B_onoff.K9B_onoff[index].K9B_BtKey[i] = 0;
		}
		k9b_save_flash_onoff();
	}
}

void RD_K9B_Save_OnOff(uint32_t mac, uint8_t key)
{
	if (((1 == key) || (2 == key) || (4 == key) || (8 == key) || (16 == key) || (32 == key) || (40 == key)))
	{
		if (Sw_Working_Stt_Val.Pair_KP9OnOff_Flag == 0x01 && Sw_Working_Stt_Val.Button_ID_OnOff < NUM_ELEMENT)
		{
			s8 pos_check_mac = check_mac_onoff_save_yet(Sw_Working_Stt_Val.Button_ID_OnOff, mac);
			if (pos_check_mac == -1)
			{
				ESP_LOGW("K9B", "save data onoff, mac: %lu, key: %02x", mac, key);
				save_data_onoff_one_btn(Sw_Working_Stt_Val.Button_ID_OnOff, mac, key);
			}
			else
			{
				ESP_LOGE("K9B", "del data onoff, mac: %lu, key: %02x", mac, key);
				k9b_delete_onoff(Sw_Working_Stt_Val.Button_ID_OnOff, pos_check_mac);
			}
			k9b_clear_pair_onoff();
		}
	}
}

static uint8_t K9B_scan_press_onoff_one_btn(uint8_t btn_index, uint32_t mac, uint8_t key, uint32_t counter, uint8_t type_k9b)
{
	uint8_t stt_return = 0;
	static struct button_value_onoff btn_pair_onoff[NUM_ELEMENT] = {0};
	if (btn_pair_onoff[btn_index].last_counter_onoff == 0x00 || btn_pair_onoff[btn_index].last_counter_onoff != counter)
	{

		for (uint8_t i = 0; i < flash_K9B_onoff.K9B_onoff[btn_index].pos_save_onoff_next; i++)
		{
			if ((key == flash_K9B_onoff.K9B_onoff[btn_index].K9B_BtKey[i] || (key == 0x01 && type_k9b == 6)) && mac == flash_K9B_onoff.K9B_onoff[btn_index].MacK9B[i])
			{
				// ESP_LOGW("K9B","ELEMENT: %d, key: %02x, type deivice: %02x", btn_index, key, type_k9b);
				if (esp_timer_get_time() - btn_pair_onoff[btn_index].time_tongle_onoff >= TIME_OUT_PRESS)
				{
					if (key == 0x01 && type_k9b == 6)
					{
						// off all
						stt_return = 0xff;
					}
					else
					{
						// tongle;
						uint8_t stt = !get_stt_present(btn_index);
						control_set_onoff(btn_index, stt);
			
						stt_return = 1;
					}
					btn_pair_onoff[btn_index].last_counter_onoff = counter;
					btn_pair_onoff[btn_index].time_tongle_onoff = esp_timer_get_time();
				}
			}
		}
	}
	return stt_return;
}

uint8_t RD_K9B_ScanPress_K9BOnOff(uint32_t mac, uint8_t key, uint32_t counter, uint8_t type_k9b)
{
	uint8_t scan_stt = 0;
	if ((key & 0x01) && type_k9b == 6)
		key = 0x01;
	for (uint8_t i = 0; i < NUM_ELEMENT; i++)
	{
		scan_stt = K9B_scan_press_onoff_one_btn(i, mac, key, counter, type_k9b);
	}
	return scan_stt;
}