/**@file    	drv_common.h
 * @brief   	��Ŀ����ͷ�ļ�
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

// <o> ֧�ִ洢�Ŀ������� - ELOCK_SAVE_CARD_NUM <10-1000>
// <i> ֧�ִ洢�Ŀ���������ÿ�ſ�4�ֽ�
#define ELOCK_SAVE_CARD_NUM    	( 100 )

// <o> ID����ȡ���ʱ�� - ELOCK_CARD_ID_INTERVAL <200-5000>
// <i> ID����ȡ���ʱ��
#define ELOCK_CARD_ID_INTERVAL	( 1000 )


// <<< end of configuration section >>>

	


/**@brief �豸ȫ�ֲ�����Ϣ�ṹ�� */
typedef struct
{
	uint8_t card_handle_state;		///< �豸��������״̬
	uint8_t card_id_have_read;		///< ��ID����ȡ�ɹ�
	uint8_t card_id_value[4];		///< �ݴ浱ǰ��ȡ��ID�����к�
	uint8_t card_ic_have_read;		///< ��IC����ȡ�ɹ�
	uint8_t card_ic_value[4];		///< �ݴ浱ǰ��ȡ��IC�����к�
	
	uint8_t elock_open_en;			///< ��������־�����ڰ�������������
	
} ELOCK_info_t;

/**@brief �Ž������� */
typedef enum {
	CARD_STATE_VERIFY = 0,	///< �豸�����鿨״̬
	CARD_STATE_ADD,			///< �豸����¼��״̬
	CARD_STATE_DELETE, 		///< �豸����ɾ��״̬
} CARD_STATUS_t;

/**@brief �Ž����Ų������� */
typedef enum {
	CARD_NUM_FLASH_GET,		///< ��flash���ºſ⵽�ڴ�
	CARD_NUM_FLASH_PUT,		///< ���ڴ���ºſ⵽flash
	CARD_NUM_FLASH_CLEAR,	///< ��պſ�
	CARD_NUM_ADD,			///< ������������
	CARD_NUM_DELETE, 		///< ��������ɾ��
	CARD_NUM_VERIFY			///< ����������֤
} CARD_OPS_TYPE_t;

/**@brief �Ž����Ų���ִ��״̬ */
typedef enum {
	CARD_OK = 0,		///< ִ�гɹ�
	CARD_NO_CARD,		///< û��Ҫ�����Ŀ�
	CARD_NO_MEM,		///< �濨��
} ELOCK_HANDLE_RESULT_t;


extern ELOCK_info_t		g_elock_info;

void elock_card_init(void);
uint8_t elock_card_ops(CARD_OPS_TYPE_t card_ops, uint8_t *card_num, uint8_t flash_update_flag);
uint8_t elock_card_process(void);

uint32_t hextoDs(uint8_t *hex);



#endif	// __DRV_COMMON_H


/******************* (C) COPYRIGHT 2019 FiberHome *****END OF FILE****/

