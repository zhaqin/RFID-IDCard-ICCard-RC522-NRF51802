/**@file    	usr_elock.c
 * @brief   	�û��������ӿ�
 * @details  	����RS485��������LED�����������г̡���������
 * @author  	wanghuan  any question please send mail to 371463817@qq.com
 * @date    	2019-11-08
 * @version 	V1.0
 * @copyright	Copyright (c) 2019-2022  ���ͨ��
 **********************************************************************************
 * @par �޸���־:
 * <table>
 * <tr><th>Date        <th>Version  <th>Author    <th>Description
 * <tr><td>2019/11/08  <td>1.0      <td>wanghuan  <td>����app_fifo_uart�⣬�����������LED����
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

// ���ڷ��͡�����FIFO��С
#define UART_TX_BUF_SIZE	256      	///< ���ڷ��ͻ����С.
#define UART_RX_BUF_SIZE   	256      	///< ���ڽ��ջ����С.


/**@brief FSUͨ�Žӿ���Ϣ�ṹ�� */
typedef struct
{
    uint8_t fsu_rs485_addr;				///< �ӻ���ַ
    uint8_t fsu_frame_buffer[202];		///< FSU����֡����
    uint16_t fsu_frame_length;			///< FSU����֡����
    uint8_t fsu_rx_flag;				///< FSU����֡���ձ�־

} FSU_RS485_info_t;

static FSU_RS485_info_t g_fsu_rs485_info;


/**@brief ���ڽ����¼������� \n
* ���ڽ������ݲ�����ģ��ָ��Э����֡��Ȼ�����me3616����ע��Ļص�������
* @param[in]  *p_event 	���������¼�����ָ��
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

        // TODO:FSU�Ž�Э��������
        // FSU�Ž�Э�飨�ϸ����ȼ�Э����ԣ�
        // ��ʼλ0x7E + ��ַ��1�ֽڣ�+ ���ȣ�2�ֽڣ�+ ���ݣ�N�ֽڣ�+ У�� + ����λ0x0D
        switch (index)
        {
        case 0:
        {
            if (rx_data == 0x7E)	//��֡ͷ
                g_fsu_rs485_info.fsu_frame_buffer[index++] = rx_data;
            else
                index = 0;
        }
        break;

        case 1:
        {
            if (rx_data == g_fsu_rs485_info.fsu_rs485_addr)	//��֤��ַ
            {
                g_fsu_rs485_info.fsu_frame_buffer[index++] = rx_data;
            }
            else
                index = 0;
        }
        break;

        default:    //�������
        {
            if(index>=2 && index<=3)	//����
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
    case APP_UART_TX_EMPTY:		//���ڷ������
        nrf_gpio_pin_write(RS485_DIR, 0);	//ת��Ϊ����
        break;
#endif

    case APP_UART_COMMUNICATION_ERROR:	//���ڽ���ͨ�ų���
        APP_ERROR_HANDLER(p_event->data.error_communication);
        break;

    case APP_UART_FIFO_ERROR:			//���ڽ���FIFOģ�����
        APP_ERROR_HANDLER(p_event->data.error_code);
        break;

    default:
        break;
    }
}

/**@brief RS485���ڳ�ʼ�� \n
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
        .flow_control = APP_UART_FLOW_CONTROL_DISABLED,	//���ؽ�ֹ
        .use_parity   = false,							//��У��
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

/**@brief ���ڷ��Ͷ��ֽ� \n
* @param[in]  *buffer 	�������ݻ���ָ��
* @param[in]  len 		���������ֽ���
*/
void drv_lock_uart_SendN( uint8_t *buffer, uint16_t len)
{
    app_uart_flush();	//�建��
    nrf_gpio_pin_write(RS485_DIR, 1);	//ʹ�ܷ���

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
    app_uart_put(0x00);	//ʹ��RS485+fifo���յ�fifo��ʱ�ر�485���ͻ�Ӱ�����һ�ֽڵķ���
#endif
}

/**@brief FSUͨ�Ž���֡Э�鴦�� \n
*/
void drv_lock_uart_recv_handle(void)
{
    if(g_fsu_rs485_info.fsu_rx_flag == 1)	//��fsuһ֡����
    {
        uint8_t fsu_rply[100];
        g_fsu_rs485_info.fsu_rx_flag = 0;
        memcpy(fsu_rply, g_fsu_rs485_info.fsu_frame_buffer, 6);

        switch( (uint16_t)(((uint16_t)(g_fsu_rs485_info.fsu_frame_buffer[5]<<8)) |
                           g_fsu_rs485_info.fsu_frame_buffer[4]) )
        {
        case 0x1001:	//����
        {
            buzzer_no_delay(400);
            drv_lock_led_set(BSP_INDICATE_ELOCK_STATE_ON, 0);
            drv_lock_open_set(LOCK_ON_KEY, 60000);	//����
            fsu_rply[6] = 0;
            fsu_rply[7] = 0x0D;
            drv_lock_uart_SendN(fsu_rply, 8);
        }
        break;

        case 0x1002:	//������
        {
            if(g_fsu_rs485_info.fsu_frame_buffer[6] == 0x00)		//�鿨ģʽ
            {
                g_elock_info.card_handle_state = CARD_STATE_VERIFY;
                LOG_PRINT("CARD_STATE_VERIFY set...\r\n");
            }
            else if(g_fsu_rs485_info.fsu_frame_buffer[6] == 0x01)	//¼��ģʽ
            {
                g_elock_info.card_handle_state = CARD_STATE_ADD;
                LOG_PRINT("CARD_STATE_ADD set...\r\n");
            }
            else if(g_fsu_rs485_info.fsu_frame_buffer[6] == 0x02)	//ɾ��ģʽ
            {
                g_elock_info.card_handle_state = CARD_STATE_DELETE;
                LOG_PRINT("CARD_STATE_DELETE set...\r\n");
            }

            fsu_rply[6] = 0;
            fsu_rply[7] = 0x0D;
            drv_lock_uart_SendN(fsu_rply, 8);
        }
        break;

        case 0x1003:	//�����λ
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

/**@brief ���������¼�������
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
        // TODO:�����ſ���
        if(g_elock_info.elock_open_en == 1)
            drv_lock_open_set(LOCK_ON, 1000);	//1sû���͹���
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

    case BSP_EVENT_LOCK_STA_RELEASE:	//����
    {
        drv_lock_open_set(LOCK_OFF, NULL);	//ֹͣ���������,�忪����־
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





APP_TIMER_DEF(m_led_delay_timer_id);	///< LED��ʱ��ʱ��
APP_TIMER_DEF(elock_open_timer_id);		///< ��������δ�򿪳�ʱ��ʱ��

/**@brief LED״̬��ʱ��ʱ������
*/
static void led_delay_timerout(void * p_context)
{
    bsp_indication_set(BSP_INDICATE_ELOCK_STATE_OFF);	//��̬Ϊ����
}

/**@brief ���ų�ʱ������������������絫û�п��ţ�
 */
static void elock_open_timerout(void * p_context)
{
    drv_lock_led_set(BSP_INDICATE_ELOCK_STATE_OFF, 0);
    drv_lock_open_set(LOCK_OFF, NULL);		//ֹͣ���������
}

/**@brief ��������IO��LED����������IO��ʼ��
*/
void drv_lock_io_init(void)
{
    ret_code_t err_code;

    // ����������LED���� BSP_INIT_BUTTONS
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

    // ��������IO
    nrf_gpio_cfg_output(LOCK_OPEN_EN);
    nrf_gpio_pin_write(LOCK_OPEN_EN, 0);
}

/**
 * @brief LED��������
 * @param[in]  indicate	LEDҪָʾ��״̬
 * @param[in]  ms 		ָʾ״̬����ʱ�䣨0Ϊ���ޣ�
 */
void drv_lock_led_set(bsp_indication_t indicate, uint32_t ms)
{
    bsp_indication_set(indicate);
    if(ms)
        app_timer_start(m_led_delay_timer_id, APP_TIMER_TICKS(ms, 0), NULL);
}

/**
 * @brief ������������
 * @param[in]  onoff	����״̬
 * @param[in]  ms 		������ʱʱ��
 */
void drv_lock_open_set(ELOCK_OPEN_STATE_t onoff, uint32_t ms)
{
    if((ms < 20000) && (ms > 300000))
        ms = 60000;
    if(onoff == LOCK_ON)		//ֱ�ӿ���
    {
        nrf_gpio_pin_write(LOCK_OPEN_EN, 1);
        app_timer_start(elock_open_timer_id, APP_TIMER_TICKS(ms, 0), NULL);
    }
    else if(onoff == LOCK_OFF)	//����
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







