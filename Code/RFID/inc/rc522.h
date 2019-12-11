/**@file    rc522.h
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

#ifndef __RC522_H
#define __RC522_H	

/////////////////////////////////////////////////////////////////////
//MF522命令字
/////////////////////////////////////////////////////////////////////
#define PCD_IDLE              0x00               ///< 取消当前命令
#define PCD_AUTHENT           0x0E               ///< 验证密钥
#define PCD_RECEIVE           0x08               ///< 接收数据
#define PCD_TRANSMIT          0x04               ///< 发送数据
#define PCD_TRANSCEIVE        0x0C               ///< 发送并接收数据
#define PCD_RESETPHASE        0x0F               ///< 复位
#define PCD_CALCCRC           0x03               ///< CRC计算

/////////////////////////////////////////////////////////////////////
//Mifare_One卡片命令字
/////////////////////////////////////////////////////////////////////
#define PICC_REQIDL           0x26               ///< 寻天线区内未进入休眠状态
#define PICC_REQALL           0x52               ///< 寻天线区内全部卡
#define PICC_ANTICOLL1        0x93               ///< 防冲撞
#define PICC_ANTICOLL2        0x95               ///< 防冲撞
#define PICC_AUTHENT1A        0x60               ///< 验证A密钥
#define PICC_AUTHENT1B        0x61               ///< 验证B密钥
#define PICC_READ             0x30               ///< 读块
#define PICC_WRITE            0xA0               ///< 写块
#define PICC_DECREMENT        0xC0               ///< 扣款
#define PICC_INCREMENT        0xC1               ///< 充值
#define PICC_RESTORE          0xC2               ///< 调块数据到缓冲区
#define PICC_TRANSFER         0xB0               ///< 保存缓冲区中数据
#define PICC_HALT             0x50               ///< 休眠

/////////////////////////////////////////////////////////////////////
//MF522 FIFO长度定义
/////////////////////////////////////////////////////////////////////
#define DEF_FIFO_LENGTH       64                 ///< FIFO size=64byte
#define MAXRLEN 			  18				 //

/////////////////////////////////////////////////////////////////////
//MF522寄存器定义
/////////////////////////////////////////////////////////////////////
// PAGE 0
#define     RFU00                 0x00    
#define     CommandReg            0x01		///< 启动或停止命令的执行
#define     ComIEnReg             0x02    	///< 中断请求
#define     DivlEnReg             0x03    	///< 中断请求
#define     ComIrqReg             0x04    	///< 包含中断请求标志
#define     DivIrqReg             0x05
#define     ErrorReg              0x06    	///< 错误标志
#define     Status1Reg            0x07    	///< 通信状态
#define     Status2Reg            0x08    	///< 发送、接收器状态
#define     FIFODataReg           0x09		///< 64位FIFO缓冲的输入输出
#define     FIFOLevelReg          0x0A		///< FIFO存储的字节数
#define     WaterLevelReg         0x0B		///< FIFO溢出深度
#define     ControlReg            0x0C		///< 控制寄存器
#define     BitFramingReg         0x0D		///< 位的帧调节
#define     CollReg               0x0E		///< RF检测到的第一个位冲突位置
#define     RFU0F                 0x0F
// PAGE 1     
#define     RFU10                 0x10
#define     ModeReg               0x11		///< 发送和接收的常用模式
#define     TxModeReg             0x12		///< 发送过程的传输速率
#define     RxModeReg             0x13		///< 接收传输速率
#define     TxControlReg          0x14		///< 控制天线驱动引脚 TX1和TX2的逻辑特性
#define     TxAutoReg             0x15		///< 天线驱动器设置
#define     TxSelReg              0x16		///< 选择天线驱动器的内部源
#define     RxSelReg              0x17		///< 内部接收器设置
#define     RxThresholdReg        0x18		///< 位译码器的阈值
#define     DemodReg              0x19		///< 解调器设置
#define     RFU1A                 0x1A
#define     RFU1B                 0x1B
#define     MifareReg             0x1C		///< 控制ISO 14443/MIFARE 模式的106kbit/s通信
#define     RFU1D                 0x1D
#define     RFU1E                 0x1E
#define     SerialSpeedReg        0x1F		///< UART速率
// PAGE 2    
#define     RFU20                 0x20  
#define     CRCResultRegM         0x21
#define     CRCResultRegL         0x22
#define     RFU23                 0x23
#define     ModWidthReg           0x24
#define     RFU25                 0x25
#define     RFCfgReg              0x26
#define     GsNReg                0x27
#define     CWGsCfgReg            0x28
#define     ModGsCfgReg           0x29
#define     TModeReg              0x2A		///< 内部定时器设置
#define     TPrescalerReg         0x2B
#define     TReloadRegH           0x2C		///< 16位定时器重装值
#define     TReloadRegL           0x2D
#define     TCounterValueRegH     0x2E		///< 16位定时器实际值
#define     TCounterValueRegL     0x2F
// PAGE 3      
#define     RFU30                 0x30
#define     TestSel1Reg           0x31		///< 测试信号配置
#define     TestSel2Reg           0x32
#define     TestPinEnReg          0x33
#define     TestPinValueReg       0x34
#define     TestBusReg            0x35
#define     AutoTestReg           0x36		///< 数字自测试
#define     VersionReg            0x37		///< 显示版本
#define     AnalogTestReg         0x38
#define     TestDAC1Reg           0x39  
#define     TestDAC2Reg           0x3A   
#define     TestADCReg            0x3B   
#define     RFU3C                 0x3C   
#define     RFU3D                 0x3D   
#define     RFU3E                 0x3E   
#define     RFU3F		  		  0x3F

/////////////////////////////////////////////////////////////////////
//和MF522通讯时返回的错误代码
/////////////////////////////////////////////////////////////////////
#define MI_OK                          0
#define MI_NOTAGERR                    (1)
#define MI_ERR                         (2)

#define	SHAQU1		0X01
#define	KUAI4		0X04
#define	KUAI7		0X07
#define	REGCARD		0xa1
#define	CONSUME		0xa2
#define READCARD	0xa3
#define ADDMONEY	0xa4


/////////////////////////////////////////////////////////////////////
//函数原型
/////////////////////////////////////////////////////////////////////
void WriteRawRC(unsigned char Address,unsigned char value);
unsigned char ReadRawRC(unsigned char Address);
void SetBitMask(unsigned char reg,unsigned char mask);
void ClearBitMask(unsigned char reg,unsigned char mask);

void PcdAntennaOn(void);
void PcdAntennaOff(void);
void PcdReset(void);
void PcdConfigISOType(unsigned char ucType);
char PcdComMF522(unsigned char Command, unsigned char *pInData, unsigned char InLenByte,
                 unsigned char *pOutData, unsigned int  *pOutLenBit);
char PcdRequest(unsigned char req_code, unsigned char *pTagType);   
char PcdAnticoll(unsigned char *pSnr);
void CalulateCRC(unsigned char *pIndata, unsigned char len, unsigned char *pOutData);
char PcdSelect(unsigned char *pSnr);         
char PcdAuthState(unsigned char auth_mode, unsigned char addr, unsigned char *pKey, unsigned char *pSnr);     
char PcdRead(unsigned char addr, unsigned char *pData);     
char PcdWrite(unsigned char addr, unsigned char *pData);    
char PcdValue(unsigned char dd_mode, unsigned char addr, unsigned char *pValue);   
char PcdBakValue(unsigned char sourceaddr, unsigned char goaladdr);                                 
char PcdHalt(void);


// 应用接口函数注册
typedef void (* spi_write_callback_t)(unsigned char const *, unsigned char);
typedef unsigned char (* spi_read_callback_t)(unsigned char);
typedef void (* spi_rst_callback_t)(unsigned char);
typedef void (* spi_delay_callback_t)(unsigned int);

void rc522_regist_init(spi_write_callback_t, spi_read_callback_t, spi_rst_callback_t, spi_delay_callback_t);				 


#endif	// __RC522_H



/******************* (C) COPYRIGHT 2019 FiberHome *****END OF FILE****/

