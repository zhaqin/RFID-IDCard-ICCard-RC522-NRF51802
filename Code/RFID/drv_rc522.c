/**@file    drv_rc522.c
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
#include <stdio.h>
#include "drv_rc522.h"
#include "nrf_gpio.h"
#include "nrf_drv_spi.h"
#include "nrf_delay.h"
#include "log.h"
#include "drv_common.h"

// <<< Use Configuration Wizard in Context Menu >>>\n
//  <q> RC522_SPI_SIMULATE_EN
//  <i> 1:enable  0:disable
#define RC522_SPI_SIMULATE_EN   0

// <o> ELOCK_CARD_IC_INTERVAL <20-2000>
// <i> ELOCK_CARD_IC_INTERVAL
#define ELOCK_CARD_IC_INTERVAL	( 200 )
// <<< end of configuration section >>>


#if (RC522_SPI_SIMULATE_EN)
#define RC522_DELAY()  			nrf_delay_us(2)	//用于调节SPI通信速率

#define RC522_CS_Enable()		nrf_gpio_pin_write(IC_SS_PIN, 0)
#define RC522_CS_Disable()		nrf_gpio_pin_write(IC_SS_PIN, 1)
#define RC522_Reset_Enable()	nrf_gpio_pin_write(IC_RST_PIN, 0)
#define RC522_Reset_Disable()	nrf_gpio_pin_write(IC_RST_PIN, 1)
#define RC522_SCK_0()			nrf_gpio_pin_write(IC_SCK_PIN, 0)
#define RC522_SCK_1()			nrf_gpio_pin_write(IC_SCK_PIN, 1)
#define RC522_MOSI_0()			nrf_gpio_pin_write(IC_MOSI_PIN, 0)
#define RC522_MOSI_1()			nrf_gpio_pin_write(IC_MOSI_PIN, 1)
#define RC522_MISO_GET()		nrf_gpio_pin_read(IC_MISO_PIN)

/**
 * @brief  向RC522发送1 Byte 数据
 * @param[in] byte  要发送的数据
 * @retval RC522返回的数据
 */
static void SPI_RC522_SendByte(uint8_t byte)
{
    uint8_t counter;

    for(counter=0; counter<8; counter++)
    {
        if(byte & 0x80)
            RC522_MOSI_1();
        else
            RC522_MOSI_0();

        RC522_DELAY();
        RC522_SCK_0();

        RC522_DELAY();
        RC522_SCK_1();

        RC522_DELAY();
        byte <<= 1;
    }
}

/**
  * @brief  从RC522接收1 Byte 数据
  * @param  无
  * @retval RC522返回的数据
  */
static uint8_t SPI_RC522_ReadByte(void)
{
    uint8_t counter;
    uint8_t SPI_Data;

    for(counter=0; counter<8; counter++)
    {
        SPI_Data <<= 1;
        RC522_SCK_0 ();

        RC522_DELAY();
        if (RC522_MISO_GET() == 1)
            SPI_Data |= 0x01;

        RC522_DELAY();
        RC522_SCK_1 ();

        RC522_DELAY();
    }
    return SPI_Data;
}

/**@brief SPI写操作
 * @param[in] *pdata  要写入的数据
 * @param[in] size    要写入的数据的长度
 * @return 	none
 */
static void rc522_spi_write(uint8_t const * pdata, uint8_t size)
{
    uint8_t i;

    RC522_CS_Enable();
    for(i=0; i<size; i++)
        SPI_RC522_SendByte(pdata[i]);
    RC522_CS_Disable();
}

/**@brief SPI读操作（单字节）
 * @param[in] data  要写入的数据
 * @return 	读取的数据
 */
static uint8_t rc522_spi_read(uint8_t data)
{
    uint8_t ucReturn;

    RC522_CS_Enable();
    SPI_RC522_SendByte(data);
    ucReturn = SPI_RC522_ReadByte();
    RC522_CS_Disable();

    return ucReturn;
}

#else	//使用硬件SPI接口
static const nrf_drv_spi_t ic_spi = NRF_DRV_SPI_INSTANCE(1);	///< spi实例
static volatile bool spi_xfer_done;	///< SPI传输完成标志

/**@brief SPI传输完成事件处理函数
 * @param[in] p_event  事件类型 NRF_DRV_SPI_EVENT_DONE
 * @return 	none
 */
static void spi_event_handler(nrf_drv_spi_event_t p_event)
{
    spi_xfer_done = true;
}

/**@brief SPI写操作
 * @param[in] *pdata  要写入的数据
 * @param[in] size    要写入的数据的长度
 * @return 	none
 */
static void rc522_spi_write(uint8_t const * pdata, uint8_t size)
{
    uint8_t r_buff[2];
    spi_xfer_done = false;
    nrf_drv_spi_transfer(&ic_spi, pdata, size, r_buff, size);
    while(!spi_xfer_done);
}

/**@brief SPI读操作（单字节）
 * @param[in] data  要写入的数据（地址）
 * @return 	读取的数据
 */
static uint8_t rc522_spi_read(uint8_t data)
{
    uint8_t rx_buff[2];
    uint8_t tx_buff[2];
    tx_buff[0] = data;
    tx_buff[1] = 0x00;
    spi_xfer_done = false;
    nrf_drv_spi_transfer(&ic_spi, tx_buff, 2, rx_buff, 2);	//读操作发送最后字节为0x00，接收首字节无效
    while(!spi_xfer_done);
    return rx_buff[1];
}
#endif

/**@brief SPI读操作（单字节）
 * @param[in] data  要写入的数据
 * @return 	读取的数据
 */
static void rc522_rst(uint8_t data)
{
    nrf_gpio_pin_write(IC_RST_PIN, data);
}

/**@brief SPI读操作（单字节）
 * @param[in] data  要写入的数据
 * @return 	读取的数据
 */
static void rc522_delay(uint32_t us)
{
    nrf_delay_us(us);
}


APP_TIMER_DEF(m_ic_read_interval_timer_id);	///< RC522读卡间隔定时器
volatile static uint8_t ic_read_flag;

/**@brief RC522读卡间隔超时处理函数
*/
static void ic_read_interval_timerout(void * p_context)
{
    ic_read_flag = 1;
}

/**@brief rc522使能初始化
 */
void rc522_enable_init(void)
{
    nrf_gpio_cfg_output(IC_RST_PIN);	// RC522复位引脚
    nrf_gpio_pin_write(IC_RST_PIN, 1);

#if(RC522_SPI_SIMULATE_EN)
    nrf_gpio_cfg_output(IC_SS_PIN);
    nrf_gpio_cfg_input(IC_MISO_PIN, NRF_GPIO_PIN_NOPULL);
    nrf_gpio_cfg_output(IC_MOSI_PIN);
    nrf_gpio_cfg_output(IC_SCK_PIN);
#else
    // spi配置及初始化
    nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG(1);
    spi_config.ss_pin   = IC_SS_PIN;
    spi_config.miso_pin = IC_MISO_PIN;
    spi_config.mosi_pin = IC_MOSI_PIN;
    spi_config.sck_pin  = IC_SCK_PIN;
    spi_config.irq_priority = APP_IRQ_PRIORITY_LOW;
    spi_config.frequency = NRF_DRV_SPI_FREQ_1M;
    spi_config.orc = 0xFF;
    spi_config.mode = NRF_DRV_SPI_MODE_0;
    spi_config.bit_order = NRF_DRV_SPI_BIT_ORDER_MSB_FIRST;	//高位在前
    APP_ERROR_CHECK(nrf_drv_spi_init(&ic_spi, &spi_config, spi_event_handler));
#endif

    rc522_regist_init(rc522_spi_write, rc522_spi_read, rc522_rst, rc522_delay);	// spi接口函数注册

    uint32_t err_code = app_timer_create(&m_ic_read_interval_timer_id, APP_TIMER_MODE_REPEATED, ic_read_interval_timerout);
    APP_ERROR_CHECK(err_code);

    PcdReset();				//芯片复位及初始化配置
    PcdConfigISOType('A');	//设置工作方式

    app_timer_start(m_ic_read_interval_timer_id, APP_TIMER_TICKS(ELOCK_CARD_IC_INTERVAL, 0), NULL);	//读IC 200ms

//	LOG_PRINT("init rc522 complete, VersionReg is %02x  \r\n", ReadRawRC(VersionReg));	//0x92
//	LOG_PRINT("ModeReg is %02x  \r\n", ReadRawRC(ModeReg));				//0x3D
//	LOG_PRINT("TxControlReg is %02x  \r\n", ReadRawRC(TxControlReg));	//0x83
}


/**
  * @brief  UID卡复制卡号方法
  * @param[in] number  要写入的卡号
  * @return 	
*/
uint8_t rc522_uid_copy()
{
    unsigned char inDoor[16] = {
    						0x60, 0xAB, 0xAC, 0xB0, 0xD7, 0x08, 0x04, 0x00, 
    						0x01, 0xD5, 0x86, 0x0F, 0xB7, 0xAD, 0xEB, 0x1D
    					};
//                            60 AB AC B0 D7 8 4 0 1 D5 86 F B7 AD EB 1D
                            
    unsigned int  unLen;
    unsigned char ucComMF522Buf[MAXRLEN];
    unsigned char i;

    //校验位计算
    for(i = 0, inDoor[4] = 0; i < 4; i++){
        inDoor[4] ^= inDoor[i];
    }

	PcdHalt();
    
	WriteRawRC(BitFramingReg, 0x07);
	ucComMF522Buf[0] = 0x40;
	PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,1,ucComMF522Buf,&unLen);
    
	WriteRawRC(BitFramingReg, 0x00);
	ucComMF522Buf[0] = 0x43;
    WriteRawRC(CommandReg, 0x40);
	PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,1,ucComMF522Buf,&unLen);
   
    return PcdWrite(0, inDoor);

}


/**
  * @brief  低功耗使用关闭接口
  */
void rc522_disable(void)
{
    // TODO:IC读卡低功耗功能待完成
}

void rc522_read(void)
{
    uint8_t IC_SN[4];	// 先后存放IC卡的类型和UID(IC卡序列号)
    uint8_t status;  	// 返回状态
    uint8_t passwd[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t read_data[16] = {0};
    uint8_t write_data[16] = {0xA0, 0xA1, 0xA2, 0xA3, 
                              0xA4, 0xA5, 0xA6, 0xA7,
                              0xA8, 0xA9, 0xAA, 0xAB,
                              0xAC, 0xAD, 0xAE, 0xAF,};
    static uint8_t i=0, j=0;
    if(ic_read_flag)
    {
//        ic_read_flag = 0;
        status = MI_ERR;
        if((status = PcdRequest(PICC_REQIDL, IC_SN)) != MI_OK)	//寻卡 PICC_REQIDL PICC_REQALL
            status = PcdRequest(PICC_REQIDL, IC_SN);			//若失败再次寻卡

        if(status == MI_OK)
        {
            // 防冲撞（当有多张卡进入读写器操作范围时，防冲突机制会从其中选择一张进行操作）
            //LOG_PRINT("IC card class is:%x%x\r\n", IC_SN[0], IC_SN[1]);
            if(PcdAnticoll(IC_SN) == MI_OK)
            {

                if(PcdSelect(IC_SN) == MI_OK)	// 选卡
               {
                    if(i < 64 && PcdAuthState(0x60, i, passwd, IC_SN) == MI_OK)
                    {
//                        if(i && (i+1)%4 != 0 && PcdWrite(i, write_data) == MI_OK){
//
//                        }
                        if(PcdRead(i, read_data) == MI_OK){
                            LOG_PRINT("block:%d ", i);
                            for(j = 0; j < 16; j++){
                                LOG_PRINT("%x ", read_data[j]);
                            }LOG_PRINT("\r\n");
                            if(((i+1) % 4) == 0){
                                LOG_PRINT("\r\n\r\n");
                            }
                            i++;
                            if(i == 64){
                                LOG_PRINT(">>>end read card block<<<\r\n");
                                i = 0;
                                if(PcdHalt() == MI_OK)	//卡休眠
                                {
                                    ic_read_flag = 0;
                                    //交由elock_card_process处理
                                    memcpy(g_elock_info.card_ic_value, IC_SN, 4);
                                    g_elock_info.card_ic_have_read = 1;
                                }
                            }

                        }
                    }
               }

//                    if(PcdHalt() == MI_OK)	//卡休眠
//                    {
////                         //交由elock_card_process处理
////                        memcpy(g_elock_info.card_ic_value, IC_SN, 4);
////                        g_elock_info.card_ic_have_read = 1;
//                    }
                }
            }
        }
}



#if 0
/**
  * @brief  IC卡测试函数
  * @param  无
  * @retval 无
  */
void IC_test(void)
{
    char cStr[30];
    uint8_t IC_SN[4];	// 先后存放IC卡的类型和UID(IC卡序列号)
    uint8_t IC_KEY_A[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
//	uint8_t IC_KEY_B[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    uint8_t IC_RFID[16];// IC卡内容
    uint8_t status;  	// 返回状态

    while(1)
    {
        status = MI_ERR;
        if((status = PcdRequest(PICC_REQIDL, IC_SN)) != MI_OK)	//寻卡 PICC_REQIDL PICC_REQALL
            status = PcdRequest(PICC_REQIDL, IC_SN);			//若失败再次寻卡

        if(status == MI_OK)
        {
            // 防冲撞（当有多张卡进入读写器操作范围时，防冲突机制会从其中选择一张进行操作）
            if(PcdAnticoll(IC_SN) == MI_OK)
            {
                sprintf(cStr, "The Card ID is: %02X%02X%02X%02X",  \
                        IC_SN[0], IC_SN[1], IC_SN[2], IC_SN[3]);
                LOG_PRINT("%s\r\n", cStr);

                status = MI_ERR;
                status = PcdSelect(IC_SN);	// 选卡
            }
        }

        if(status == MI_OK)	//选卡成功
        {
            status = MI_ERR;
            status = PcdAuthState(0x60, 11, IC_KEY_A, IC_SN);	// 验证卡片密码A,默认0xFFFFFFFFFFFF
        }

        if(status == MI_OK)	//验证成功
        {
            status = MI_ERR;
            status = PcdRead(11, IC_RFID);// 读卡（2区块3）
        }

        if(status == MI_OK)	//读卡成功
        {
            LOG_PRINT("The Card ID is:");
            uint8_t i;
            for(i=0; i<16; i++)
                LOG_PRINT("%02X", IC_RFID[i]);
            LOG_PRINT("\r\n");

            PcdHalt();	//卡休眠
        }

        nrf_delay_ms(100);
    }
}
#endif


/******************* (C) COPYRIGHT 2019 FiberHome *****END OF FILE****/

