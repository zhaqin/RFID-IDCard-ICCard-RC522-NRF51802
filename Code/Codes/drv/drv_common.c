

#include "drv_common.h"

ELOCK_info_t g_elock_info;
static uint8_t card_sn[ELOCK_SAVE_CARD_NUM][4];				///< 门禁卡存储
static uint8_t empty_card[4] = {0xFF, 0xFF, 0xFF, 0xFF};	///< 空卡



/**@brief 门禁卡号验证函数  \n
 * 		  即查询卡号是否在号库中
 * @param[in] card_num		要操作的卡号
 * @param[out] card_pos		要操作的卡号的序号
 * @return 	true 成功， false 失败
 */
static bool elock_card_verify(uint8_t *card_num, uint8_t *card_pos)
{
    // TODO:等待falsh驱动完成
    uint8_t i;

    for(i=0; i<ELOCK_SAVE_CARD_NUM; i++)
    {
        if(0 == memcmp(card_sn[i], card_num, 4))
        {
            *card_pos = i;
            return true;
        }
    }

    return false;
}

/**@brief 门禁卡号操作函数  \n
 * 		  包含录卡、删卡、验证卡
 * @param[in] card_handle	卡号操作类型
 * @param[in] card_num		要操作的卡号
 * @return 	号卡操作的状态
 */
uint8_t elock_card_ops(CARD_OPS_TYPE_t card_ops, uint8_t *card_num, uint8_t flash_update_flag)
{
    uint8_t card_pos;

    switch (card_ops)
    {
    case CARD_NUM_FLASH_GET:
    {
        drv_flash_read(CARD_NUM_FLASH_ADDR, card_sn[0], ELOCK_SAVE_CARD_NUM*4);
    }
    break;

    case CARD_NUM_FLASH_PUT:
    {
        drv_flash_write(CARD_NUM_FLASH_ADDR, card_sn[0], ELOCK_SAVE_CARD_NUM*4);
        LOG_PRINT("All card write to flash over...\r\n");
    }
    break;

    case CARD_NUM_FLASH_CLEAR:
    {
        memset(card_sn[0], 0xFF, ELOCK_SAVE_CARD_NUM*4);
        drv_flash_write(CARD_NUM_FLASH_ADDR, card_sn[0], ELOCK_SAVE_CARD_NUM*4);
        LOG_PRINT("clear flash over...\r\n");
    }
    break;

    case CARD_NUM_ADD:
    {
        if(!elock_card_verify(card_num, &card_pos))	//不在号库才增加
        {
            if(!elock_card_verify(empty_card, &card_pos))	//寻找空卡
                return CARD_NO_MEM;
            memcpy(card_sn[card_pos], card_num, 4);//存入到空卡位置
            drv_flash_write(card_pos*4, card_num, 4);
        }
    }
    break;

    case CARD_NUM_DELETE:
    {
        if(elock_card_verify(card_num, &card_pos))	//在号库才删除
        {
            memcpy(card_sn[card_pos], empty_card, 4);//置空卡
            drv_flash_write(card_pos*4, empty_card, 4);
        }
        else
            return CARD_NO_CARD;

    }
    break;

    case CARD_NUM_VERIFY:
    {
        if(!elock_card_verify(card_num, &card_pos))
            return CARD_NO_CARD;
    }
    break;

    default:
        break;
    }

    return CARD_OK;
}


APP_TIMER_DEF(card_id_interval_timer_id);	//ID读卡间隔计时器
static void card_id_interval_timerout(void * p_context);


/**@brief ID读卡间隔定时器超时处理函数
 */
static void card_id_interval_timerout(void * p_context)
{
    drv_id_read();
}

/**@brief 门禁卡号操作初始化  \n
 * 		  主要是使用定时器的初始化
 */
void elock_card_init(void)
{
    ret_code_t err_code;

    err_code = app_timer_create(&card_id_interval_timer_id, APP_TIMER_MODE_SINGLE_SHOT, card_id_interval_timerout);
    APP_ERROR_CHECK(err_code);

    drv_flash_init();	// flash驱动初始化
    elock_card_ops(CARD_NUM_FLASH_GET, NULL, NULL);	//从flash获取卡号库
    drv_id_read();	//使能首次读ID卡
}

/**@brief 门禁卡号操作函数  \n
 * 		  包含录卡、删卡、验证卡
 * @param[in] card_handle	卡号操作类型
 * @param[in] card_num		要操作的卡号
 * @return 	号卡操作的状态
 */
uint8_t elock_card_process(void)
{
    if(g_elock_info.card_id_have_read)
    {
        g_elock_info.card_id_have_read = 0;
        LOG_PRINT("ID card: %02X%02X%02X%02X\r\n", \
                  g_elock_info.card_id_value[0], g_elock_info.card_id_value[1], \
                  g_elock_info.card_id_value[2], g_elock_info.card_id_value[3]);


        if(g_elock_info.card_handle_state == CARD_STATE_VERIFY)		//验证状态
        {
            if(CARD_OK == elock_card_ops(CARD_NUM_VERIFY, g_elock_info.card_id_value, NULL))	//验卡OK
            {
                buzzer_no_delay(400);
                drv_lock_led_set(BSP_INDICATE_ELOCK_STATE_ON, 0);
                // TODO: 开门操作改按键才开
                drv_lock_open_set(LOCK_ON_KEY, 60000);	//按键开锁
                LOG_PRINT("ID card verify OK...\r\n");
            }
            else
            {
                drv_lock_led_set(BSP_INDICATE_ELOCK_STATE_FAST, 800);
                LOG_PRINT("ID card verify FAIL...\r\n");
            }
        }
        else if(g_elock_info.card_handle_state == CARD_STATE_ADD)	//录卡状态
        {
            if(CARD_OK == elock_card_ops(CARD_NUM_ADD, g_elock_info.card_id_value, NULL))
            {
                buzzer_no_delay(400);
                LOG_PRINT("ID card add OK...\r\n");
            }
            else
            {
                drv_lock_led_set(BSP_INDICATE_ELOCK_STATE_FAST, 800);
                LOG_PRINT("ID card add FAIL...\r\n");
            }
        }
        else if(g_elock_info.card_handle_state == CARD_STATE_DELETE)//删卡状态
        {
            if(CARD_OK == elock_card_ops(CARD_NUM_DELETE, g_elock_info.card_id_value, NULL))
            {
                buzzer_no_delay(400);
                LOG_PRINT("ID card delete OK...\r\n");
            }
            else
            {
                drv_lock_led_set(BSP_INDICATE_ELOCK_STATE_FAST, 800);
                LOG_PRINT("ID card delete FAIL...\r\n");
            }
        }
        app_timer_start(card_id_interval_timer_id, APP_TIMER_TICKS(ELOCK_CARD_ID_INTERVAL, 0), NULL);	//ID读卡间隔500ms
    }

    if(g_elock_info.card_ic_have_read)
    {
        g_elock_info.card_ic_have_read = 0;
        LOG_PRINT("IC card: %02X%02X%02X%02X\r\n", \
                  g_elock_info.card_ic_value[0], g_elock_info.card_ic_value[1], \
                  g_elock_info.card_ic_value[2], g_elock_info.card_ic_value[3]);

        if(g_elock_info.card_handle_state == CARD_STATE_VERIFY)		//验证状态
        {
            if(CARD_OK == elock_card_ops(CARD_NUM_VERIFY, g_elock_info.card_ic_value, NULL))	//验卡OK
            {
                buzzer_no_delay(400);
                drv_lock_led_set(BSP_INDICATE_ELOCK_STATE_ON, 0);
                drv_lock_open_set(LOCK_ON_KEY, 60000);	//按键触发开锁（有效等待按键时间60s）
                LOG_PRINT("IC card verify OK...\r\n");
            }
            else
            {
                drv_lock_led_set(BSP_INDICATE_ELOCK_STATE_FAST, 800);
                LOG_PRINT("IC card verify FAIL...\r\n");
            }
        }
        else if(g_elock_info.card_handle_state == CARD_STATE_ADD)	//录卡状态
        {
            if(CARD_OK == elock_card_ops(CARD_NUM_ADD, g_elock_info.card_ic_value, NULL))
            {
                buzzer_no_delay(400);
                LOG_PRINT("IC card add OK...\r\n");
            }
            else
            {
                drv_lock_led_set(BSP_INDICATE_ELOCK_STATE_FAST, 800);
                LOG_PRINT("IC card add FAIL...\r\n");
            }
        }
        else if(g_elock_info.card_handle_state == CARD_STATE_DELETE)//删卡状态
        {
            if(CARD_OK == elock_card_ops(CARD_NUM_DELETE, g_elock_info.card_ic_value, NULL))
            {
                buzzer_no_delay(400);
                LOG_PRINT("IC card delete OK...\r\n");
            }
            else
            {
                drv_lock_led_set(BSP_INDICATE_ELOCK_STATE_FAST, 800);
                LOG_PRINT("IC card delete FAIL...\r\n");
            }
        }
    }

    return 0;
}


/**
* @brief 16进制转十进制
*/
uint32_t hextoDs(uint8_t *hex)
{
    uint8_t i = 0;
    uint16_t num = 0;
    bool flags = false;
    uint32_t sum = 0, pow = 1;

    for(i = 9; i >= 4; i--) {
        num = hex[i] <= 0x09 ? hex[i] : hex[i] - 0x0A + 10;
        sum += flags ? num * (pow *= 16) : num;
        flags = true;
    }
    return sum;
}


/******************* (C) COPYRIGHT 2019 FiberHome *****END OF FILE****/

