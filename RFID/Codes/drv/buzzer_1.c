#include "buzzer.h"
#include "app_timer.h"
//#include "nrf_drv_rtc.h"
//#include "nrf_drv_clock.h"

APP_TIMER_DEF(buzzer_timer_id);			//���������嶨ʱ��
APP_TIMER_DEF(buzzer_delay_timer_id);	//��������ʱ��ʱ��

static void buzzer_timerout(void * p_context)
{
	nrf_gpio_pin_toggle(BZ_EN);
}

static void buzzer_delay_timerout(void * p_context)
{
	buzzer_onoff(false);
}

/**
* @brief ������������ʼ��  \n
* ʹ�������ʱ������תIO
*/
void buzzer_init(void)
{
	ret_code_t err_code;
	err_code = app_timer_create(&buzzer_timer_id, APP_TIMER_MODE_REPEATED, buzzer_timerout);
	APP_ERROR_CHECK(err_code);
	err_code = app_timer_create(&buzzer_delay_timer_id, APP_TIMER_MODE_SINGLE_SHOT, buzzer_delay_timerout);
	APP_ERROR_CHECK(err_code);
	
	nrf_gpio_cfg_output(BZ_EN);
}

/**
* @brief ����������
*/
void buzzer_onoff(bool onoff)
{
	if(onoff){
		app_timer_start(buzzer_timer_id, 6, NULL);	//32768/8 = 4096 Hz
	}else{
		app_timer_stop(buzzer_timer_id);
	}
}

/**
* @brief ��������ʱ����
*/
void buzzer_no_delay(uint32_t ms)
{
	buzzer_onoff(true);
	app_timer_start(buzzer_delay_timer_id, APP_TIMER_TICKS(ms, 0), NULL);
}


#if 0
/**
* ����������  \n
* ���������ʱ����RTC1��ʹ�ã��˷�������
*/
const nrf_drv_rtc_t buzzer_rtc = NRF_DRV_RTC_INSTANCE(1);	//RTC1ʵ��

static void rtc1_handler(nrf_drv_rtc_int_type_t int_type)
{
	LOG_PRINT("rtc int\r\n");
    if (int_type == NRF_DRV_RTC_INT_COMPARE0)
    {
        LOG_PRINT("rtc cc0\r\n");
    }
    else if (int_type == NRF_DRV_RTC_INT_TICK)
    {
        LOG_PRINT("rtc tick\r\n");
    }
}


/**
* @brief ������������ʼ��  \n
* ʹ��rtc�¼�--ppi--gpio��ת����
*/
void buzzer_init(void)
{
	ret_code_t err_code;
	
	err_code = nrf_drv_clock_init(NULL);
    APP_ERROR_CHECK(err_code);
    nrf_drv_clock_lfclk_request();
	
	// RTC����
	nrf_drv_rtc_config_t rtc_config = NRF_DRV_RTC_DEFAULT_CONFIG(1);
	rtc_config.prescaler = 4095;	// 32768/1 = 32768 Hz
	err_code = nrf_drv_rtc_init(&buzzer_rtc, &rtc_config, rtc1_handler);
    APP_ERROR_CHECK(err_code);
	nrf_drv_rtc_tick_enable(&buzzer_rtc, true);
	err_code = nrf_drv_rtc_cc_set(&buzzer_rtc, 0, 8, true);	//���ñȽ�ֵ
	APP_ERROR_CHECK(err_code);
	nrf_drv_rtc_enable(&buzzer_rtc);
	LOG_PRINT("rtc init over...\r\n");
	
#if 0	
	// ������GPIOTE����
	nrf_drv_gpiote_out_config_t io_config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false);
	err_code = nrf_drv_gpiote_out_init(BZ_EN, &io_config);
	APP_ERROR_CHECK(err_code);
	
	// ppi����
	err_code = nrf_drv_ppi_channel_alloc(&ppi_channe2);	//ppiͨ������,��һ����Ƶppi���˵�ַ
	APP_ERROR_CHECK(err_code);
	err_code = nrf_drv_ppi_channel_assign(ppi_channe2, 
					nrf_drv_gpiote_in_event_addr_get(BZ_EN),
					nrf_drv_rtc_event_address_get(&buzzer_rtc, NRF_RTC_EVENT_COMPARE_0));
	APP_ERROR_CHECK(err_code);
	
	
	err_code = nrf_drv_ppi_channel_enable(ppi_channe2);	//ʹ��ppiͨ��
	APP_ERROR_CHECK(err_code);
	nrf_drv_gpiote_out_task_enable(BZ_EN);	//ʹ��gpiote�������
	nrf_drv_rtc_disable(&buzzer_rtc);
#endif
}


void buzzer_onoff(bool onoff)
{
	if(onoff){
		nrf_drv_rtc_enable(&buzzer_rtc);
	}else{
		nrf_drv_rtc_disable(&buzzer_rtc);
	}
}

void buzzer_no_delay(uint32_t ms)
{
	buzzer_onoff(true);
	nrf_delay_ms(ms);
	buzzer_onoff(false);
}
#endif

