
#ifndef _USR_ELOCK_H_
#define _USR_ELOCK_H_

#include <stdint.h>
#include "bsp.h"

//// lock
//#define LOCK_OPEN_EN				8	///< 开锁
//#define LOCK_LED					13	///< 指示灯
//#define LOCK_KEY					14	///< 按键
//#define LOCK_STA					15	///< 锁状态
//#define LOCK_DOOR					25	///< 门状态


//// FSU
//#define FSU_DO					29
//#define RS485_TX					24
//#define RS485_RX					22
//#define RS485_DIR					23

/**@brief 开锁状态 */
typedef enum {
	LOCK_OFF = 0,		///< 关锁
	LOCK_ON = 1,		///< 直接开锁
	LOCK_ON_KEY,		///< 按键开锁，用于开锁标志有效计时
} ELOCK_OPEN_STATE_t;


uint32_t drv_lock_key_status(void);
uint32_t drv_lock_status(void);
uint32_t drv_lock_door_status(void);
uint32_t drv_lock_fsu_do_status(void);


void drv_lock_io_init(void);
void drv_lock_led_set(bsp_indication_t indicate, uint32_t ms);
void drv_lock_open_set(ELOCK_OPEN_STATE_t onoff, uint32_t ms);

// 与FSU通信
void drv_lock_uart_init(void);
void drv_lock_uart_SendN( uint8_t *buffer, uint16_t num);
void drv_lock_uart_recv_handle(void);

#endif


