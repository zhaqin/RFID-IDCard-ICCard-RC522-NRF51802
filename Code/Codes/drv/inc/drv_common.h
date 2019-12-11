/**@file    	drv_common.h
 * @brief   	项目公共头文件
 * @details  	
 * @author  	
 * @date    	2019-11-06
 * @version 	V1.0
 * @copyright	Copyright (c) 2019-2022  烽火通信
 **********************************************************************************
 * @par 修改日志:
 * <table>
 * <tr><th>Date        <th>Version  <th>Author    <th>Description
 * <tr><td>2019/11/06  <td>1.0      <td>wanghuan  <td>创建初始版本
 * </table>
 */

#ifndef __DRV_COMMON_H
#define __DRV_COMMON_H
#include <stdint.h>
#include "nrf.h"
#include "bsp.h"
#include "softdevice_handler.h"
#include "drv_flash.h"
#include "log.h"
#include "buzzer.h"
#include "app_timer.h"

// <<< Use Configuration Wizard in Context Menu >>>\n

// <o> 支持存储的卡号数量 - ELOCK_SAVE_CARD_NUM <10-1000>
// <i> 支持存储的卡号数量，每张卡4字节
#define ELOCK_SAVE_CARD_NUM    	( 100 )

// <o> ID卡读取间隔时间 - ELOCK_CARD_ID_INTERVAL <200-5000>
// <i> ID卡读取间隔时间
#define ELOCK_CARD_ID_INTERVAL	( 1000 )


// <<< end of configuration section >>>

	


/**@brief 设备全局参数信息结构体 */
typedef struct
{
	uint8_t card_handle_state;		///< 设备操作卡的状态
	uint8_t card_id_have_read;		///< 有ID卡读取成功
	uint8_t card_id_value[4];		///< 暂存当前读取的ID卡序列号
	uint8_t card_ic_have_read;		///< 有IC卡读取成功
	uint8_t card_ic_value[4];		///< 暂存当前读取的IC卡序列号
	
	uint8_t elock_open_en;			///< 门锁开标志（用于按键触发开锁）
	
} ELOCK_info_t;

/**@brief 门禁卡类型 */
typedef enum {
	CARD_STATE_VERIFY = 0,	///< 设备处于验卡状态
	CARD_STATE_ADD,			///< 设备处于录卡状态
	CARD_STATE_DELETE, 		///< 设备处于删卡状态
} CARD_STATUS_t;

/**@brief 门禁卡号操作类型 */
typedef enum {
	CARD_NUM_FLASH_GET,		///< 从flash更新号库到内存
	CARD_NUM_FLASH_PUT,		///< 从内存更新号库到flash
	CARD_NUM_FLASH_CLEAR,	///< 清空号库
	CARD_NUM_ADD,			///< 开锁卡号增加
	CARD_NUM_DELETE, 		///< 开锁卡号删除
	CARD_NUM_VERIFY			///< 开锁卡号验证
} CARD_OPS_TYPE_t;

/**@brief 门禁卡号操作执行状态 */
typedef enum {
	CARD_OK = 0,		///< 执行成功
	CARD_NO_CARD,		///< 没有要操作的卡
	CARD_NO_MEM,		///< 存卡满
} ELOCK_HANDLE_RESULT_t;


extern ELOCK_info_t		g_elock_info;

void elock_card_init(void);
uint8_t elock_card_ops(CARD_OPS_TYPE_t card_ops, uint8_t *card_num, uint8_t flash_update_flag);
uint8_t elock_card_process(void);

uint32_t hextoDs(uint8_t *hex);



#endif	// __DRV_COMMON_H


/******************* (C) COPYRIGHT 2019 FiberHome *****END OF FILE****/

