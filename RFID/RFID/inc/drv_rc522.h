/**@file    drv_rc522.h
 * @brief   	RC522驱动程序
 * @details  
 * @author  	wanghuan  any question please send mail to 371463817@qq.com
 * @date    	2019-11-04
 * @version 	V1.0
 * @copyright	Copyright (c) 2019-2022  烽火通信
 **********************************************************************************
 * @par 修改日志:
 * <table>
 * <tr><th>Date        <th>Version  <th>Author    <th>Description
 * <tr><td>2019/11/04  <td>1.0      <td>wanghuan  <td>创建初始版本
 * </table>
 */

/**@defgroup drv_rc522 Drv rc522 driver module.
 * @{
 * @ingroup bsp_drivers
 * @brief 使用该驱动之前，先进行驱动句柄的实例注册. \n
 * spi 
 */

#ifndef __DRV_RC522_H
#define __DRV_RC522_H
#include "rc522.h"
#include "drv_common.h"


//#define IC_SCK_PIN		11
//#define IC_MOSI_PIN		10
//#define IC_MISO_PIN		9
//#define IC_SS_PIN		12
//#define IC_RST_PIN		16


void rc522_enable_init(void);
void rc522_read(void);
uint8_t rc522_uid_copy(void);
void IC_test(void);

#endif	// __DRV_RC522_H


/** @} drv_rc522*/

/******************* (C) COPYRIGHT 2019 FiberHome *****END OF FILE****/
