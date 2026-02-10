/*
 * utils.c
 *
 *  Created on: Feb 6, 2026
 *      Author: PC5
 */

#include "tl_common.h"

#include "../inc/utils.h"


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
//	LOGV("size ring: %d", rdRingBuf.fill_size);
	return rdRingBuf.fill_size;
}



/******************************* Dynamic Allocation **********************************/

#define HEAP_SIZE 1024

typedef struct block {
    uint32_t size;
    uint8_t  free;
    struct block *next;
} block_t;

static uint8_t heap[HEAP_SIZE];
static block_t *heap_head;


void heap_init(void)
{
    heap_head = (block_t *)heap;
    heap_head->size = HEAP_SIZE - sizeof(block_t);
    heap_head->free = 1;
    heap_head->next = NULL;
}

void *rd_malloc(uint32_t size)
{
    block_t *curr = heap_head;

    while (curr) {
        if (curr->free && curr->size >= size) {

            if (curr->size > size + sizeof(block_t)) {
                block_t *new_block = (block_t *)((uint8_t *)curr + sizeof(block_t) + size);
                new_block->size = curr->size - size - sizeof(block_t);
                new_block->free = 1;
                new_block->next = curr->next;

                curr->next = new_block;
                curr->size = size;
            }

            curr->free = 0;
            return (uint8_t *)curr + sizeof(block_t);
        }
        curr = curr->next;
    }

    return NULL;
}

void *rd_calloc(uint32_t n, uint32_t size)
{
    uint32_t total = n * size;
    void *ptr = rd_malloc(total);
    if (!ptr) return NULL;

    memset(ptr, 0, total);
    return ptr;
}

void *rd_realloc(void *ptr, uint32_t new_size)
{
    if (!ptr) return rd_malloc(new_size);

    block_t *block = (block_t *)((uint8_t *)ptr - sizeof(block_t));

    if (block->size >= new_size) {
        return ptr;
    }

    void *new_ptr = rd_malloc(new_size);
    if (!new_ptr) return NULL;

    memcpy(new_ptr, ptr, block->size);
    rd_free(ptr);

    return new_ptr;
}

void rd_free(void *ptr)
{
    if (!ptr) return;

    block_t *block = (block_t *)((uint8_t *)ptr - sizeof(block_t));
    block->free = 1;

    block_t *curr = heap_head;
    while (curr && curr->next) {
        if (curr->free && curr->next->free) {
            curr->size += sizeof(block_t) + curr->next->size;
            curr->next = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}

















