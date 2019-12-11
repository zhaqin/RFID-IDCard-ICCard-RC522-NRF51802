/**@file    	drv_flash.c
 * @brief   	flash��������
 * @details  	
 * @author  	
 * @date    	2019-11-06
 * @version 	V1.0
 * @copyright	Copyright (c) 2019-2022  ���ͨ��
 **********************************************************************************
 * @par �޸���־:
 * <table>
 * <tr><th>Date        <th>Version  <th>Author    <th>Description
 * <tr><td>2019/11/06  <td>1.0      <td>wanghuan  <td>������ʼ�汾
 * </table>
 */
#ifndef __DRV_FLASH_H
#define __DRV_FLASH_H
#include <string.h>
#include "nrf.h"
#include "bsp.h"
#include "softdevice_handler.h"
#include "fstorage.h"

// <<< Use Configuration Wizard in Context Menu >>>\n

// <o> FALSH�����汾 - FLASH_DRV_VERSION
// <i> FALSH�����Ĳ�ͬ�汾
//      <1=> FLASH_DRV_NVMC
//      <2=> FLASH_DRV_FSTORAGE
//      <3=> FLASH_DRV_FDS
#define FLASH_DRV_VERSION    	( 2 )

// <o> FALSH����ʹ��ҳ�� - FLASH_DRV_USE_PAGES <1-10>
// <i> FALSH����ʹ��ҳ��
#define FLASH_DRV_USE_PAGES		( 1 )

// <<< end of configuration section >>>

typedef enum {
	FLASH_DRV_NVMC 			= 1,		///< ������Э��ջʱ��nvmc����
	FLASH_DRV_FSTORAGE		= 2, 		///< ����sd�ӿڵ� fstorage
	FLASH_DRV_FDS			= 3,		///< ����fstorage�ӿڵ� fds
} FALSH_DRV_TYPE_t;


#define CARD_NUM_FLASH_ADDR		0




void drv_flash_init(void);
ret_code_t drv_flash_write(uint32_t addr, uint8_t *data, uint16_t len);
void drv_flash_read(uint32_t addr, uint8_t *data, uint16_t len);



#endif	// __DRV_FLASH_H

/******************* (C) COPYRIGHT 2019 FiberHome *****END OF FILE****/

