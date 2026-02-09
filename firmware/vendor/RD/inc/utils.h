/*
 * utils.h
 *
 *  Created on: Feb 6, 2026
 *      Author: PC5
 */

#ifndef UTILS_H_
#define UTILS_H_

#include "proj/common/types.h"
#include "proj_lib/ble/blt_config.h"

/************************** LOG *******************************/

typedef enum {
	LOG_NONE, /*!< No log output */
	LOG_ERROR, /*!< Critical errors, software module can not recover on its own */
	LOG_WARN, /*!< Error conditions from which recovery measures have been taken */
	LOG_INFO, /*!< Information messages which describe normal flow of events */
	LOG_DEBUG, /*!< Extra information which is not necessary for normal use (values, pointers, sizes, etc). */
	LOG_VERBOSE /*!< Bigger chunks of debugging information, or frequent messages which can potentially flood the output. */
} log_level_t;

void log_write(const char *format, ...);

#define LOGE(fmt, ...) \
    do { \
        if (log_level >= LOG_ERROR) { \
            log_write("[ERR] " fmt "\n",  ##__VA_ARGS__); \
        } \
    } while (0)

#define LOGW(fmt, ...) \
    do { \
        if (log_level >= LOG_WARN) { \
            log_write("[WAR] " fmt "\n", ##__VA_ARGS__); \
        } \
    } while (0)

#define LOGI(fmt, ...) \
    do { \
        if (log_level >= LOG_INFO) { \
            log_write("[INF] " fmt "\n", ##__VA_ARGS__); \
        } \
    } while (0)

#define LOGD(fmt, ...) \
    do { \
        if (log_level >= LOG_DEBUG) { \
            log_write("[DBG] " fmt "\n", ##__VA_ARGS__); \
        } \
    } while (0)

#define LOGV(fmt, ...) \
    do { \
        if (log_level >= LOG_VERBOSE) { \
            log_write("[VER] " fmt "\n", ##__VA_ARGS__); \
        } \
    } while (0)


#define LOG_HEX_BUFF(buf, len)                         \
do {                                                   \
    for (int i = 0; i < (int)(len); i++) {              \
    	log_write("%02X ", ((uint8_t *)(buf))[i]);          \
    }                                                  \
    log_write("\n");                                        \
} while(0)


extern log_level_t log_level;
inline void log_set_level(log_level_t level){
	log_level = level;
}
void soft_uart_init(GPIO_PinTypeDef tx_pin, uint16_t baudrate);
void soft_uart_send_data(char* data);

/*==================RING BUFFER====================*/
typedef struct{
    uint8_t *buffer;
    uint16_t fill_size;
    uint16_t buf_size;
    uint16_t head;
    uint16_t tail;
} ring_buffer_t;

#if 0
void ring_buffer_init(ring_buffer_t *rb, uint8_t *buff, uint8_t size_buf);
int ring_buffer_put(ring_buffer_t *rb, uint8_t data);
int ring_buffer_get(ring_buffer_t *rb, uint8_t *data);
void ring_buffer_flush(ring_buffer_t *rb);
inline uint16_t ring_buffer_get_size(ring_buffer_t *rb) {return rb->fill_size;}
#endif

void rd_buffer_init(void);
int rd_buffer_put_data(uint8_t data);
int rd_buffer_get_data(uint8_t *data, uint16_t len);
void rd_flush(void);
uint16_t rd_buffer_get_size(void);


#endif /* UTILS_H_ */
