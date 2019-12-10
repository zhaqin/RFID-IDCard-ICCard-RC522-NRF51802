/**@file    	usr_elock.c
 * @brief   	用户电子锁接口
 * @details  	包含RS485、按键、LED、门锁、门行程、蜂鸣器等
 * @author  	wanghuan  any question please send mail to 371463817@qq.com
 * @date    	2019-11-08
 * @version 	V1.0
 * @copyright	Copyright (c) 2019-2022  烽火通信
 **********************************************************************************
 * @par 修改日志:
 * <table>
 * <tr><th>Date        <th>Version  <th>Author    <th>Description
 * <tr><td>2019/11/08  <td>1.0      <td>wanghuan  <td>采用app_fifo_uart库，加入蜂鸣器、LED控制
 * </table>
 */
#include "usr_elock.h"
#include "log.h"
#include "nordic_common.h"
#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"
#include "app_uart.h"
#include "nrf_drv_uart.h"
#include "nrf_delay.h"
#include "drv_common.h"

#define UART_USE_RS485	1

// 串口发送、接收FIFO大小
#define UART_TX_BUF_SIZE	256      	///< 串口发送缓冲大小.
#define UART_RX_BUF_SIZE   	256      	///< 串口接收缓冲大小.


/**@brief FSU通信接口信息结构体 */
typedef struct
{
    uint8_t fsu_rs485_addr;				///< 从机地址
    uint8_t fsu_frame_buffer[202];		///< FSU数据帧缓存
    uint16_t fsu_frame_length;			///< FSU数据帧长度
    uint8_t fsu_rx_flag;				///< FSU数据帧接收标志

} FSU_RS485_info_t;

static FSU_RS485_info_t g_fsu_rs485_info;


/**@brief 串口接收事件处理函数 \n
* 串口接收数据并根据模组指令协议组帧，然后调用me3616驱动注册的回调处理函数
* @param[in]  *p_event 	串口外设事件类型指针
*/
static void fsu_uart_event_handle(app_uart_evt_t * p_event)
{
    static uint8_t index = 0;
    uint8_t rx_data;

    switch (p_event->evt_type)
    {
    case APP_UART_DATA_READY:
    {
        UNUSED_VARIABLE( app_uart_get(&rx_data) );

        // TODO:FSU门禁协议需完善
        // FSU门禁协议（较复杂先简单协议测试）
        // 起始位0x7E + 地址（1字节）+ 长度（2字节）+ 数据（N字节）+ 校验 + 结束位0x0D
        switch (index)
        {
        case 0:
        {
            if (rx_data == 0x7E)	//找帧头
                g_fsu_rs485_info.fsu_frame_buffer[index++] = rx_data;
            else
                index = 0;
        }
        break;

        case 1:
        {
            if (rx_data == g_fsu_rs485_info.fsu_rs485_addr)	//验证地址
            {
                g_fsu_rs485_info.fsu_frame_buffer[index++] = rx_data;
            }
            else
                index = 0;
        }
        break;

        default:    //其他情况
        {
            if(index>=2 && index<=3)	//长度
            {
                g_fsu_rs485_info.fsu_frame_buffer[index++] = rx_data;
                if(index == 3)
                    g_fsu_rs485_info.fsu_frame_length = g_fsu_rs485_info.fsu_frame_buffer[2] +
                                                        g_fsu_rs485_info.fsu_frame_buffer[3]*16;
            }
            else if(index>=4 && rx_data!=0x0D)
            {
                g_fsu_rs485_info.fsu_frame_buffer[index++] = rx_data;
            }
            else if(index>=4 && rx_data==0x0D)
            {
                g_fsu_rs485_info.fsu_frame_buffer[index] = rx_data;
                index = 0;
                g_fsu_rs485_info.fsu_rx_flag = 1;
            }
        }
        break;

        }

    }
    break;

#if (UART_USE_RS485)
    case APP_UART_TX_EMPTY:		//串口发送完成
        nrf_gpio_pin_write(RS485_DIR, 0);	//转换为接收
        break;
#endif

    case APP_UART_COMMUNICATION_ERROR:	//串口接收通信出错
        APP_ERROR_HANDLER(p_event->data.error_communication);
        break;

    case APP_UART_FIFO_ERROR:			//串口接收FIFO模块出错
        APP_ERROR_HANDLER(p_event->data.error_code);
        break;

    default:
        break;
    }
}

/**@brief RS485串口初始化 \n
*/
void drv_lock_uart_init(void)
{
    uint32_t err_code;

    app_uart_comm_params_t const comm_params =
    {
        .rx_pin_no    = RS485_RX,  	//RX_PIN_NUMBER
        .tx_pin_no    = RS485_TX,  	//TX_PIN_NUMBER
        .rts_pin_no   = NULL,
        .cts_pin_no   = NULL,
        .flow_control = APP_UART_FLOW_CONTROL_DISABLED,	//流控禁止
        .use_parity   = false,							//无校验
        .baud_rate    = NRF_UART_BAUDRATE_9600
    };

    APP_UART_FIFO_INIT(&comm_params,		\
                       UART_RX_BUF_SIZE,	\
                       UART_TX_BUF_SIZE,	\
                       fsu_uart_event_handle,	\
                       APP_IRQ_PRIORITY_HIGH,	\
                       err_code);
    APP_ERROR_CHECK(err_code);

    // 485 recieve mode
    nrf_gpio_cfg_output(RS485_DIR);
    nrf_gpio_pin_write(RS485_DIR, 0);

    g_fsu_rs485_info.fsu_rs485_addr = 1;

    LOG_PRINT("drv_lock_uart_init complete !\r\n");
}

/**@brief 串口发送多字节 \n
* @param[in]  *buffer 	发送数据缓冲指针
* @param[in]  len 		发送数据字节数
*/
void drv_lock_uart_SendN( uint8_t *buffer, uint16_t len)
{
    app_uart_flush();	//清缓冲
    nrf_gpio_pin_write(RS485_DIR, 1);	//使能发送

//	uint32_t err_code;
//	do{
//		err_code = nrf_drv_uart_tx(buffer, len);
//		LOG_PRINT("nrf_drv_uart_tx: %d !\r\n", err_code);
//	} while(err_code != NRF_SUCCESS);


    uint16_t i;
    for(i=0; i<len; i++)
    {
        app_uart_put(buffer[i]);
    }
#if (UART_USE_RS485)
    app_uart_put(0x00);	//使用RS485+fifo，收到fifo空时关闭485发送会影响最后一字节的发送
#endif
}

/**@brief FSU通信接收帧协议处理 \n
*/
void drv_lock_uart_recv_handle(void)
{
    if(g_fsu_rs485_info.fsu_rx_flag == 1)	//有fsu一帧数据
    {
        uint8_t fsu_rply[100];
        g_fsu_rs485_info.fsu_rx_flag = 0;
        memcpy(fsu_rply, g_fsu_rs485_info.fsu_frame_buffer, 6);

        switch( (uint16_t)(((uint16_t)(g_fsu_rs485_info.fsu_frame_buffer[5]<<8)) |
                           g_fsu_rs485_info.fsu_frame_buffer[4]) )
        {
        case 0x1001:	//开锁
        {
            buzzer_no_delay(400);
            drv_lock_led_set(BSP_INDICATE_ELOCK_STATE_ON, 0);
            drv_lock_open_set(LOCK_ON_KEY, 60000);	//开锁
            fsu_rply[6] = 0;
            fsu_rply[7] = 0x0D;
            drv_lock_uart_SendN(fsu_rply, 8);
        }
        break;

        case 0x1002:	//卡操作
        {
            if(g_fsu_rs485_info.fsu_frame_buffer[6] == 0x00)		//验卡模式
            {
                g_elock_info.card_handle_state = CARD_STATE_VERIFY;
                LOG_PRINT("CARD_STATE_VERIFY set...\r\n");
            }
            else if(g_fsu_rs485_info.fsu_frame_buffer[6] == 0x01)	//录卡模式
            {
                g_elock_info.card_handle_state = CARD_STATE_ADD;
                LOG_PRINT("CARD_STATE_ADD set...\r\n");
            }
            else if(g_fsu_rs485_info.fsu_frame_buffer[6] == 0x02)	//删卡模式
            {
                g_elock_info.card_handle_state = CARD_STATE_DELETE;
                LOG_PRINT("CARD_STATE_DELETE set...\r\n");
            }

            fsu_rply[6] = 0;
            fsu_rply[7] = 0x0D;
            drv_lock_uart_SendN(fsu_rply, 8);
        }
        break;

        case 0x1003:	//软件复位
        {
            fsu_rply[6] = 0;
            fsu_rply[7] = 0x0D;
            drv_lock_uart_SendN(fsu_rply, 8);

            nrf_delay_ms(200);
            sd_nvic_SystemReset();
        }
        break;
        }
    }
}

/**@brief 按键输入事件处理函数
*/
static void eclok_bsp_event_handler(bsp_event_t event)
{
    switch (event)
    {
    case BSP_EVENT_WAKEUP:
    {

    }
		break;

    case BSP_EVENT_LOCK_KEY_PUSH:
    {
        // TODO:按键才开锁
        if(g_elock_info.elock_open_en == 1)
            drv_lock_open_set(LOCK_ON, 1000);	//1s没开就关锁
        else
            drv_lock_led_set(BSP_INDICATE_ELOCK_STATE_ON, 500);
        LOG_PRINT("BSP_EVENT_LOCK_KEY_PUSH !\r\n");
    }
    break;

    case BSP_EVENT_LOCK_DOOR_PUSH:
    {
        LOG_PRINT("BSP_EVENT_LOCK_DOOR_COLSE !\r\n");
    }
    break;

    case BSP_EVENT_LOCK_DOOR_RELEASE:
    {
        LOG_PRINT("BSP_EVENT_LOCK_DOOR_OPEN !\r\n");
    }
    break;

    case BSP_EVENT_LOCK_STA_PUSH:
    {
        LOG_PRINT("BSP_EVENT_LOCK_STA_PUSH !\r\n");
    }
    break;

    case BSP_EVENT_LOCK_STA_RELEASE:	//锁打开
    {
        drv_lock_open_set(LOCK_OFF, NULL);	//停止电磁锁供电,清开锁标志
        drv_lock_led_set(BSP_INDICATE_ELOCK_STATE_OFF, 0);
        LOG_PRINT("BSP_EVENT_LOCK_STA_RELEASE !\r\n");
    }
    break;

    case BSP_EVENT_FSU_DO_PUSH:
    {
        LOG_PRINT("BSP_EVENT_FSU_DO_PUSH !\r\n");
    }
    break;

    case BSP_EVENT_FSU_DO_RELEASE:
    {
        LOG_PRINT("BSP_EVENT_FSU_DO_RELEASE !\r\n");
    }
    break;

    default:
        break;
    }
}





APP_TIMER_DEF(m_led_delay_timer_id);	///< LED限时定时器
APP_TIMER_DEF(elock_open_timer_id);		///< 门锁开启未打开超时定时器

/**@brief LED状态限时超时处理函数
*/
static void led_delay_timerout(void * p_context)
{
    bsp_indication_set(BSP_INDICATE_ELOCK_STATE_OFF);	//常态为常灭
}

/**@brief 开门超时处理函数（给电磁铁供电但没有开门）
 */
static void elock_open_timerout(void * p_context)
{
    drv_lock_led_set(BSP_INDICATE_ELOCK_STATE_OFF, 0);
    drv_lock_open_set(LOCK_OFF, NULL);		//停止电磁锁供电
}

/**@brief 门锁按键IO、LED及其他控制IO初始化
*/
void drv_lock_io_init(void)
{
    ret_code_t err_code;

    // 按键输入与LED配置 BSP_INIT_BUTTONS
    err_code = bsp_init(BSP_INIT_LED | BSP_INIT_BUTTONS, APP_TIMER_TICKS(100, 0), eclok_bsp_event_handler);
    APP_ERROR_CHECK(err_code);


    err_code = bsp_event_to_button_action_assign(0, BSP_BUTTON_ACTION_PUSH, BSP_EVENT_LOCK_KEY_PUSH);
    APP_ERROR_CHECK(err_code);
    err_code = bsp_event_to_button_action_assign(1, BSP_BUTTON_ACTION_PUSH, BSP_EVENT_LOCK_DOOR_PUSH);  
    APP_ERROR_CHECK(err_code);
	  err_code = bsp_event_to_button_action_assign(1, BSP_BUTTON_ACTION_RELEASE, BSP_EVENT_LOCK_DOOR_RELEASE);
    APP_ERROR_CHECK(err_code);
    err_code = bsp_event_to_button_action_assign(2, BSP_BUTTON_ACTION_PUSH, BSP_EVENT_LOCK_STA_PUSH);
    APP_ERROR_CHECK(err_code);
    err_code = bsp_event_to_button_action_assign(2, BSP_BUTTON_ACTION_RELEASE, BSP_EVENT_LOCK_STA_RELEASE);
    APP_ERROR_CHECK(err_code);
    err_code = bsp_event_to_button_action_assign(3, BSP_BUTTON_ACTION_PUSH, BSP_EVENT_FSU_DO_PUSH);
    APP_ERROR_CHECK(err_code);
    err_code = bsp_event_to_button_action_assign(3, BSP_BUTTON_ACTION_RELEASE, BSP_EVENT_FSU_DO_RELEASE);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_led_delay_timer_id, APP_TIMER_MODE_SINGLE_SHOT, led_delay_timerout);
    APP_ERROR_CHECK(err_code);
    err_code = app_timer_create(&elock_open_timer_id, APP_TIMER_MODE_SINGLE_SHOT, elock_open_timerout);
    APP_ERROR_CHECK(err_code);

    // 开锁控制IO
    nrf_gpio_cfg_output(LOCK_OPEN_EN);
    nrf_gpio_pin_write(LOCK_OPEN_EN, 0);
}

/**
 * @brief LED操作函数
 * @param[in]  indicate	LED要指示的状态
 * @param[in]  ms 		指示状态持续时间（0为无限）
 */
void drv_lock_led_set(bsp_indication_t indicate, uint32_t ms)
{
    bsp_indication_set(indicate);
    if(ms)
        app_timer_start(m_led_delay_timer_id, APP_TIMER_TICKS(ms, 0), NULL);
}

/**
 * @brief 门锁操作函数
 * @param[in]  onoff	开锁状态
 * @param[in]  ms 		开锁超时时间
 */
void drv_lock_open_set(ELOCK_OPEN_STATE_t onoff, uint32_t ms)
{
    if((ms < 20000) && (ms > 300000))
        ms = 60000;
    if(onoff == LOCK_ON)		//直接开锁
    {
        nrf_gpio_pin_write(LOCK_OPEN_EN, 1);
        app_timer_start(elock_open_timer_id, APP_TIMER_TICKS(ms, 0), NULL);
    }
    else if(onoff == LOCK_OFF)	//关锁
    {
        g_elock_info.elock_open_en = 0;
        nrf_gpio_pin_write(LOCK_OPEN_EN, 0);
    }
    else if(onoff == LOCK_ON_KEY)
    {
        g_elock_info.elock_open_en = 1;
        app_timer_start(elock_open_timer_id, APP_TIMER_TICKS(ms, 0), NULL);
    }
}






#if 0
void drv_lock_irq_handler(uint32_t n, nrf_gpiote_polarity_t action)
{
    if (n == LOCK_KEY) {
        LOG_PRINT("LOCK_KEY");
    }
}

void drv_lock_init()
{
    if (!nrf_drv_gpiote_is_init()) {
        if (NRF_SUCCESS != nrf_drv_gpiote_init()) {
            LOG_ERR(" fail to init gpio event ! \r\n");
        }
    }
    nrf_gpio_pin_pull_t pull_config = NRF_GPIO_PIN_NOPULL;
    nrf_gpio_cfg_output(LOCK_OPEN_EN);
//	nrf_gpio_cfg_output(LOCK_LED);
    nrf_gpio_cfg_input(LOCK_KEY, pull_config);
    nrf_gpio_cfg_input(LOCK_STA, pull_config);
    nrf_gpio_cfg_input(LOCK_DOOR, pull_config);
    nrf_drv_gpiote_in_config_t const config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(false);
    nrf_drv_gpiote_in_init(FSU_DO, &config, drv_lock_irq_handler);
    nrf_drv_gpiote_in_event_enable(FSU_DO, true);
}
#endif





uint32_t drv_lock_key_status()
{
    return nrf_gpio_pin_read(LOCK_KEY);
}

uint32_t drv_lock_status()
{
    return nrf_gpio_pin_read(LOCK_STA);
}

uint32_t drv_lock_door_status()
{
    return nrf_gpio_pin_read(LOCK_DOOR);
}

uint32_t drv_lock_fsu_do_status()
{
    return nrf_gpio_pin_read(FSU_DO);
}







