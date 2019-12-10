#include "drv_id.h"
#include "nrf_drv_clock.h"
#include "app_timer.h"
//#include "nrf_drv_rtc.h"
#include "drv_common.h"

// <<< Use Configuration Wizard in Context Menu >>>\n
//  <q> 使用pwm库 - DRV_PWM_LIB_EN
//  <i> 1使用新版测试， 0，使用原版
#define DRV_PWM_LIB_EN   1

//  <q> 使用硬件定时器解码 - DRV_HWTIMER_ENCODE_EN
//  <i> 1:使用， 0:使用软件定时器
#define DRV_HWTIMER_ENCODE_EN   1

//  <q> 读卡超时功能使能 - DRV_FUNC_READ_TIMEOUT_EN
//  <i> 1:使能， 0:禁止
#define DRV_FUNC_READ_TIMEOUT_EN   0
// <<< end of configuration section >>>


#if (!DRV_PWM_LIB_EN)

#else	//--#if (!DRV_PWM_TIMER_EN)
/****************** 定时器输入捕获测试代码 ***********************/
#include "app_pwm.h"

ID_VAR card = {0};

//static void drv_id_read_init(void);
static void drv_id_exit_read(void);


// 软件定时器
#if (DRV_HWTIMER_ENCODE_EN == 0)
APP_TIMER_DEF(read_seq_timer_id);	//读卡时序计时器
static void read_seq_timerout(void * p_context);
#elif (DRV_HWTIMER_ENCODE_EN == 1)
static nrf_drv_timer_t capture_timer = NRF_DRV_TIMER_INSTANCE(2);
static void capture_timer_event_handle(nrf_timer_event_t event_type, void * p_context);
#endif

#if (DRV_FUNC_READ_TIMEOUT_EN)
APP_TIMER_DEF(read_tout_timer_id);	//读卡超时计时器
static void read_tout_timerout(void * p_context);
#endif


/**
* @brief 数据校验
*/
static bool drv_col_check(uint8_t check_data){
	uint8_t i,j,cnt = 0;
	uint8_t map[4] = {0x08, 0x04, 0x02, 0x01};
	for(i = 0; i < 4; i++){
		for(j = 0; j < DATA_SIZE ; j++){
				if((card_data[j] & map[i])){
					cnt++;
				}
		}
		if(!(cnt % 2) != !(check_data & map[i])){
			return false;
		}
		cnt = 0;
	}
	return true;
}

/**
* @brief 读卡成功之后的id输出
*/
static void id_number_put(){
	if(card.data_state != READ_OK){
		return;
	}
	// 交由elock_card_process处理
	g_elock_info.card_id_value[0] = (uint8_t)((card_data[2]<<4)&0xF0) + (card_data[3]&0x0F);
	g_elock_info.card_id_value[1] = (uint8_t)((card_data[4]<<4)&0xF0) + (card_data[5]&0x0F);
	g_elock_info.card_id_value[2] = (uint8_t)((card_data[6]<<4)&0xF0) + (card_data[7]&0x0F);
	g_elock_info.card_id_value[3] = (uint8_t)((card_data[8]<<4)&0xF0) + (card_data[9]&0x0F);
	g_elock_info.card_id_have_read = 1;
	
	drv_id_exit_read();	//退出读卡，等验卡再超时启动
	card.data_state = NOT_CRAD;
	
//	card.card_num = hextoDs(card_data);
//	LOG_PRINT("\rnumber:%u\r\n", card.card_num);
//	buzzer_no_delay(400);
//	LOG_PRINT("ID card put ok\r\n");
//	drv_id_exit_read();
//	nrf_delay_ms(200);
//	drv_id_read();
}

#if (DRV_HWTIMER_ENCODE_EN == 0)
/**
* @brief 读卡时序一周期超时函数
*/
static void read_seq_timerout(void * p_context)
{
	card.tick = nrf_gpio_pin_read(ID_OUT_PIN);
	card.sta_wait = false;
	card.sta_temp = card.tick;
}
#elif (DRV_HWTIMER_ENCODE_EN == 1)
static void capture_timer_event_handle(nrf_timer_event_t event_type, void * p_context)
{
	switch (event_type){
	case NRF_TIMER_EVENT_COMPARE0:
		card.tick = nrf_gpio_pin_read(ID_OUT_PIN);
		card.sta_wait = false;
		card.sta_temp = card.tick;
//			LOG_PRINT("*");
		nrf_drv_timer_compare_int_disable(&capture_timer, NRF_TIMER_CC_CHANNEL0);
		break;

	default:
		//Do nothing.
		break;
    }
}
#endif

/**
* @brief 读卡GPIOTE的中断处理函数，对曼切斯特解码
*/
void gpiote_ent(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
	if(card.sta_wait) return;
	if( pin == ID_OUT_PIN){
		card.tick = nrf_gpio_pin_read(ID_OUT_PIN);
		if(card.sta_temp != card.tick && !card.sta_wait){
			card.sta_wait = true;	// wait 1 cycle
#if (DRV_HWTIMER_ENCODE_EN == 0)
			app_timer_start(read_seq_timer_id, 9, NULL);
#elif (DRV_HWTIMER_ENCODE_EN == 1)
			nrf_drv_timer_clear(&capture_timer);
			nrf_drv_timer_compare_int_enable(&capture_timer, NRF_TIMER_CC_CHANNEL0);
#endif
#if 0
			LOG_PRINT("%d ", card.sta_temp);
#else
			if(card.data_state == NOT_CRAD && !card.sta_temp) card.data_state = DATA_HEAD;
			// get card heda data
			if(card.data_state == DATA_HEAD){
				if(card.sta_temp){
					card.data_cnt ++;
					if(card.data_cnt == 9){
						card.data_cnt = 0;
						card.data_state = DATA_BIT;
//				LOG_PRINT("---card data head ok---\r\n");
					}
				}else{
						card.data_cnt = 0;
				}
			// card row data get and check
			}else if(card.data_state == DATA_BIT){
				card.data_cnt++;
				if(card.data_cnt < 5 && card.sta_temp)	card.bit_cnt++;
				// row check
				if(card.data_cnt == 5){
					if((card.bit_cnt % 2 == card.sta_temp)){
						card.bit_cnt = 0;
						card.data_cnt = 0;
						card_data[card.byte] = card.data;
						
						card.byte++;
						card.data = 0;
						if(card.byte >= 10){
							card.byte = 0;
							card.data_state = DATA_ECC;
						}
					}else{
						card.bit_cnt = 0;
						card.data_cnt = 0;
						card.byte = 0;
						card.data = 0;
						card.data_state = NOT_CRAD;
//						LOG_PRINT("--data failed-- \r\n");
					}
				// get i bit data
				}else if(card.data_cnt <= 4){
					card.data |= (0x0f & (card.sta_temp << (4 - card.data_cnt)));
				}
				else{
						card.bit_cnt = 0;
						card.data_cnt = 0;
						card.byte++;
						card.data = 0;
						card.data_state = NOT_CRAD;
					}
			}// column check state				
			else if(card.data_state == DATA_ECC){
					card.data_cnt ++;
					if(card.data_cnt == 5){
							card.data_cnt = 0;
							// stop bit check
						
							if(card.sta_temp){
//								LOG_PRINT("----stop check failed----\r\n");
								card.data_state = NOT_CRAD;
							//column check
							}else if(!drv_col_check(card.data)){
									card.data_state = NOT_CRAD;
//									LOG_PRINT("----card column check failed----\r\n");
							}else{
									card.data_state = READ_OK;
//							LOG_PRINT("----card read is ok----\r\n");
									id_number_put();
							}
					// get i bit data		
					}else{
						card.data |= (0x0f & (card.sta_temp << (4 - card.data_cnt)));
					}
				}
#endif
		}
		card.sta_temp = card.tick;
	}
	
}


// app_pwm库使用
APP_PWM_INSTANCE(PWM1, 1);			// 创建pwm实例，PWM1使用timer1
static volatile bool ready_flag;	// pwm状态标志
static void pwm_ready_callback(uint32_t pwm_id)    // PWM回调
{
    ready_flag = true;
//	LOG_PRINT("pwm ready...\r\n");
}


/**
* @brief 读id卡初始化  \n
* 输出pwm初始化、使用软件定时器和gpiote初始化
*/
void drv_id_init(void)
{
	ret_code_t err_code;
	
	/* 配置pwm通道，125KHz，输出到 ID_125K_PIN */
    app_pwm_config_t pwm1_cfg = APP_PWM_DEFAULT_CONFIG_1CH(8, ID_125K_PIN);
//    pwm1_cfg.pin_polarity[0] = APP_PWM_POLARITY_ACTIVE_LOW;		//切换有效极性
    err_code = app_pwm_init(&PWM1, &pwm1_cfg, pwm_ready_callback);	//初始化pwm输出
    APP_ERROR_CHECK(err_code);
	
	/* 读卡定时器与IO配置 */
#if (DRV_HWTIMER_ENCODE_EN == 0)
	err_code = app_timer_create(&read_seq_timer_id, APP_TIMER_MODE_SINGLE_SHOT, read_seq_timerout);
	APP_ERROR_CHECK(err_code);
#elif (DRV_HWTIMER_ENCODE_EN == 1)
	nrf_drv_timer_config_t capture_timer_config = NRF_DRV_TIMER_DEFAULT_CONFIG(2);
	capture_timer_config.frequency = NRF_TIMER_FREQ_1MHz;
	capture_timer_config.interrupt_priority = APP_IRQ_PRIORITY_HIGH;
	err_code = nrf_drv_timer_init(&capture_timer, &capture_timer_config, capture_timer_event_handle);
	APP_ERROR_CHECK(err_code);
	nrf_drv_timer_extended_compare(&capture_timer, NRF_TIMER_CC_CHANNEL0, 327, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, false);
	nrf_drv_timer_enable(&capture_timer);
#endif
#if (DRV_FUNC_READ_TIMEOUT_EN)
	err_code = app_timer_create(&read_tout_timer_id, APP_TIMER_MODE_SINGLE_SHOT, read_tout_timerout);
	APP_ERROR_CHECK(err_code);
#endif

	if (!nrf_drv_gpiote_is_init()) {
		if (NRF_SUCCESS != nrf_drv_gpiote_init()) {
			LOG_ERR("fail to init gpio event !");
		}
	}
	nrf_drv_gpiote_in_config_t id_in_cofig  = GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
	nrf_drv_gpiote_in_init(ID_OUT_PIN, &id_in_cofig, gpiote_ent);
	
	/* 定时器2捕获读卡 */
//	drv_id_read_init();
}

/**
* @brief 读卡输出125K
*/
void drv_id_125K_onoff(bool onoff)
{
	if(onoff){
//		ready_flag = false;
		app_pwm_enable(&PWM1);
		while (app_pwm_channel_duty_set(&PWM1, 0, 50) == NRF_ERROR_BUSY);	//设置占空比50，等待pwm就绪
//		while(!ready_flag);
	}else{
		app_pwm_disable(&PWM1);
	}
}

#if (DRV_FUNC_READ_TIMEOUT_EN)
/**
* @brief 读卡未成功超时函数
*/
static void read_tout_timerout(void * p_context)
{
	if(card.data_state != READ_OK){
//		drv_id_exit_read();
		LOG_PRINT("ID card read timeout...\r\n");
	}
}
#endif

/**
 * @brief 主动读卡函数
 * @param[in]  *card_num 	输出10进制卡号
 * @param[in]  timeout 		读卡超时时间,单位ms
 * @return  返回读卡结果
 * - 0	执行成功
 * - 1 	执行失败
 */
void drv_id_read(void)
{
//	LOG_PRINT("ID card read begin\r\n");
	memset(&card, 0, sizeof(card));
	nrf_drv_gpiote_in_event_enable(ID_OUT_PIN, true);
	drv_id_125K_onoff(true);
#if (DRV_FUNC_READ_TIMEOUT_EN)
	app_timer_start(read_tout_timer_id, APP_TIMER_TICKS(1000, 0), NULL);	//读卡操作计时
#endif
}

/**
* @brief 读卡退出
*/
static void drv_id_exit_read(void)
{
//	LOG_PRINT("exit read ID card !\r\n");
	nrf_drv_gpiote_in_event_disable(ID_OUT_PIN);
	drv_id_125K_onoff(false);
#if (DRV_HWTIMER_ENCODE_EN == 0)
	app_timer_stop(read_seq_timer_id);
#elif (DRV_HWTIMER_ENCODE_EN == 1)
	
#endif
#if (DRV_FUNC_READ_TIMEOUT_EN)
	app_timer_stop(read_tout_timer_id);
#endif
}



#if 0
static nrf_drv_timer_t capture_timer = NRF_DRV_TIMER_INSTANCE(2);
static nrf_ppi_channel_t ppi_channel_read;
static uint32_t us_count = 0;

static void capture_timer_event_handle(nrf_timer_event_t event_type, void * p_context)
{
	us_count++;
}

void gpiote_handle(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
	LOG_PRINT("%d \r\n", us_count);
}

/**
* @brief 使用ppi来实现定时器的输入捕获功能 \n
* ppi连接gpiote事件和timer任务
*/
void drv_id_read_init(void)
{
	ret_code_t err_code;
	
	if (!nrf_drv_gpiote_is_init()) {
		if (NRF_SUCCESS != nrf_drv_gpiote_init()) {
			LOG_ERR("fail to init gpio event !");
		}
	}
	
	// 捕获定时器配置
	nrf_drv_timer_config_t capture_timer_config = NRF_DRV_TIMER_DEFAULT_CONFIG(2);
	capture_timer_config.frequency = NRF_TIMER_FREQ_1MHz;	// NRF_TIMER_FREQ_125kHz
	capture_timer_config.mode = NRF_TIMER_MODE_COUNTER;	// NRF_TIMER_MODE_COUNTER
	err_code = nrf_drv_timer_init(&capture_timer, &capture_timer_config, capture_timer_event_handle);
	APP_ERROR_CHECK(err_code);
	nrf_drv_timer_extended_compare(&capture_timer, NRF_TIMER_CC_CHANNEL0, 1, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, false);
	nrf_drv_timer_compare_int_enable(&capture_timer, NRF_TIMER_CC_CHANNEL0);
	nrf_drv_timer_enable(&capture_timer);
	
	// IO设置
	nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
	err_code = nrf_drv_gpiote_in_init(ID_OUT_PIN, &in_config, gpiote_handle);
	APP_ERROR_CHECK(err_code);
	
	// ppi配置
//	err_code = nrf_drv_ppi_init();	//main里面已初始化
//	err_code = nrf_drv_ppi_channel_alloc(&ppi_channel_read);	//ppi通道分配,下一步分频ppi两端地址
//	APP_ERROR_CHECK(err_code);
//	err_code = nrf_drv_ppi_channel_assign(ppi_channel_read, 
//					nrf_drv_gpiote_in_event_addr_get(ID_OUT_PIN),
//					nrf_drv_timer_task_address_get(&capture_timer, NRF_TIMER_TASK_CAPTURE0));
//	APP_ERROR_CHECK(err_code);
//	err_code = nrf_drv_ppi_channel_enable(ppi_channel_read);
//	APP_ERROR_CHECK(err_code);
		
//	nrf_drv_timer_capture(&capture_timer, NRF_TIMER_CC_CHANNEL2);
}
#endif


#endif	//--#if (!DRV_PWM_TIMER_EN)





