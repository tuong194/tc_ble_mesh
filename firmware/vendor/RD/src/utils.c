/*
 * utils.c
 *
 *  Created on: Feb 6, 2026
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

#include "../inc/utils.h"

#define TX_PIN_LOG GPIO_PD7

typedef struct{
	GPIO_PinTypeDef tx_pin;
	uint16_t baudrate;
	uint16_t bit_time;
}soft_uart_t;
static soft_uart_t soft_uart;
log_level_t log_level = LOG_NONE;

void soft_uart_send_data(char* data);

void log_write(const char *format, ...){
    static char out[512];
    char *p = out;
    va_list args;
    va_start(args, format);
    print(&p, format, args); // ~~~ vsprintf
    va_end(args);
    *p = '\0';
    soft_uart_send_data(out);
}

void soft_uart_init(GPIO_PinTypeDef tx_pin, uint16_t baudrate)
{
	soft_uart.tx_pin = tx_pin;
	soft_uart.baudrate = baudrate;
	soft_uart.bit_time = 1000000/baudrate;

	gpio_set_func(soft_uart.tx_pin ,AS_GPIO);
	gpio_set_output_en(soft_uart.tx_pin, 1);
	gpio_set_input_en(soft_uart.tx_pin ,0);
	gpio_write(soft_uart.tx_pin, 1);
}

void soft_uart_send_byte(uint8_t data){
	gpio_write(soft_uart.tx_pin, 0);
	sleep_us(soft_uart.bit_time);

    for(int i = 0; i < 8; i++)
    {
        int bit = (data >> i) & 0x01;
        gpio_write(soft_uart.tx_pin, bit);
        sleep_us(soft_uart.bit_time);
    }
    // Stop bit
    gpio_write(soft_uart.tx_pin, 1);
    sleep_us(soft_uart.bit_time);
}

void soft_uart_send_data(char* data){
	while(*data != '\0'){
		soft_uart_send_byte(*(data++));
	}
}

/**************************RING BUFFER*******************************/
#define MAX_SIZE_BUFF 16
static ring_buffer_t rdRingBuf;
static uint8_t my_buff[MAX_SIZE_BUFF];

static inline bool ring_bufer_is_empty(ring_buffer_t *rb){
	return rb->fill_size == 0;
}

static inline bool ring_buffer_is_full(ring_buffer_t *rb){
	return rb->fill_size == rb->buf_size;
}

void ring_buffer_init(ring_buffer_t *rb, uint8_t *buff, uint8_t size_buf){
	rb->buffer = buff;
	rb->buf_size = size_buf;
	rb->fill_size = 0;
	rb->head = 0;
	rb->tail = 0;
}

int ring_buffer_put(ring_buffer_t *rb, uint8_t data){
	rb->buffer[rb->tail] = data;
	rb->tail = (rb->tail + 1) % rb->buf_size;
	if(ring_buffer_is_full(rb)){
		rb->head = (rb->head + 1) % rb->buf_size;
		return -1;
	}
	rb->fill_size++;
	return 0;
}

int ring_buffer_get(ring_buffer_t *rb, uint8_t *data){
	if(!ring_bufer_is_empty(rb)){
		*data = rb->buffer[rb->head];
		rb->head = (rb->head + 1) % rb->buf_size;
		rb->fill_size--;
		return 0;
	}
	return -1;
}

void ring_buffer_flush(ring_buffer_t *rb)
{
    rb->fill_size = 0;
    rb->head = 0;
    rb->tail = 0;
    memset(rb->buffer, 0, rb->buf_size);
}

/******************** implement rd ring buff *************************/
void rd_buffer_init(void){
	ring_buffer_init(&rdRingBuf, my_buff, MAX_SIZE_BUFF);
}

void rd_flush(void){
	LOGV("flush");
	ring_buffer_flush(&rdRingBuf);
}

int rd_buffer_put_data(uint8_t data){
	return ring_buffer_put(&rdRingBuf, data);
}

int rd_buffer_get_data(uint8_t *data, uint16_t len){
    uint16_t count = 0;
    while(count < len)
    {
        if(ring_buffer_get(&rdRingBuf, &data[count]) == 0)
        {
            count++;
        }
        else
        {
            break; // no more data
        }
    }
    return count;
}

uint16_t rd_buffer_get_size(void){
	//LOGD("size: %d", rdRingBuf.fill_size);
	return rdRingBuf.fill_size;
}





















