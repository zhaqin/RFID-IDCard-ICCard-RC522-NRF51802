/**@file    rc522.c
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
#include "rc522.h"
#include "log.h"


static spi_write_callback_t		rc522_spi_write = 0;
static spi_read_callback_t    	rc522_spi_read = 0;
static spi_rst_callback_t    	rc522_rst = 0;
static spi_delay_callback_t    	rc522_delay_us = 0;

/**
 * @brief rc522注册通信接口操作函数
 * @param[in] spi_write_callback		spi写操作
 * @param[in] spi_read_callback    		spi读操作
 * @param[in] rst_callback    			rc522复位引脚
 * @param[in] delay_callback    		延时
 * @return 	none
 */
void rc522_regist_init(spi_write_callback_t spi_write_callback,
                       spi_read_callback_t spi_read_callback,
                       spi_rst_callback_t rst_callback,
                       spi_delay_callback_t delay_callback)
{
    rc522_spi_write = spi_write_callback;
    rc522_spi_read = spi_read_callback;
    rc522_rst = rst_callback;
    rc522_delay_us = delay_callback;
}


/**
 * @brief 读RC522寄存器(单字节)
 * @param[in] Address	寄存器地址
 * @return 	读出的值
 */
unsigned char ReadRawRC(unsigned char Address)
{
    unsigned char ucAddr;
    ucAddr = ( (Address << 1) & 0x7E ) | 0x80;	//读操作

    return (rc522_spi_read(ucAddr));
}

/**
 * @brief 写RC522寄存器(2字节)
 * @param[in] Address	寄存器地址
 * @param[in] value		写入的值
 * @return 	none
 */
void WriteRawRC(unsigned char Address, unsigned char value)
{
    unsigned char ucAddr;
    unsigned char tx_buf[2];
    ucAddr = (Address << 1) & 0x7E;	//写操作
    tx_buf[0] = ucAddr;
    tx_buf[1] = value;

    rc522_spi_write(tx_buf, 2);
}

/**
 * @brief 置RC522寄存器位
 * @param[in] reg	寄存器地址
 * @param[in] mask	置位值
 * @return 	none
 */
void SetBitMask(unsigned char reg,unsigned char mask)
{
    unsigned char tmp = 0x0;
    tmp = ReadRawRC(reg);
    WriteRawRC(reg, tmp | mask);  // set bit mask
}

/**
 * @brief 清RC522寄存器位
 * @param[in] reg	寄存器地址
 * @param[in] mask	清位值
 * @return 	none
 */
void ClearBitMask(unsigned char reg,unsigned char mask)
{
    unsigned char tmp = 0x0;
    tmp = ReadRawRC(reg);
    WriteRawRC(reg, tmp & (~mask));  // clear bit mask
}

/**
 * @brief 开启天线  \n
 * 每次启动或关闭天险发射之间应至少有1ms的间隔
 */
void PcdAntennaOn(void)
{
    unsigned char uc;
    uc = ReadRawRC(TxControlReg);
    if (!(uc & 0x03))
        SetBitMask(TxControlReg, 0x03);
}

/**
 * @brief 关闭天线
 */
void PcdAntennaOff(void)
{
    ClearBitMask(TxControlReg, 0x03);
}

/**
 * @brief 复位RC522
 */
void PcdReset(void)
{
    rc522_rst(1);
    rc522_delay_us(1);
    rc522_rst(0);
    rc522_delay_us(1);
    rc522_rst(1);	//芯片复位

    WriteRawRC(CommandReg, PCD_RESETPHASE);	//复位
    while(ReadRawRC(CommandReg) & 0x10);
    rc522_delay_us(1);

    WriteRawRC(ModeReg, 0x3D);      	//定义发送和接收常用模式 和Mifare卡通讯，CRC初始值0x6363
    WriteRawRC(TReloadRegL, 30);      	//16位定时器低位 15ms
    WriteRawRC(TReloadRegH, 0);			//16位定时器高位
    WriteRawRC(TModeReg, 0x8D);			//定义内部定时器的设置(数据收发时自动启停)
    WriteRawRC(TPrescalerReg, 0x3E);	//设置定时器分频系数 6.78M/0x0D3E = 2KHz
    WriteRawRC(TxAutoReg, 0x40);		//调制发送信号为100%ASK
}

/**
  * @brief  设置RC522的工作方式
  * @param  ucType，工作方式
  * @retval 无
  */
void PcdConfigISOType(unsigned char ucType)
{
    if (ucType == 'A')              	//ISO14443_A
    {
        ClearBitMask (Status2Reg, 0x08);
        WriteRawRC(ModeReg, 0x3D);   	//定义发送和接收常用模式 和Mifare卡通讯，CRC初始值0x6363
        WriteRawRC(RxSelReg, 0x86);   	//84
        WriteRawRC(RFCfgReg, 0x7F);  	//4F 配置接收器增益 48dB
        WriteRawRC(TReloadRegL, 30);	//定时 15ms
        WriteRawRC(TReloadRegH, 0);
        WriteRawRC(TModeReg, 0x8D);		//2KHz
        WriteRawRC(TPrescalerReg, 0x3E);

        rc522_delay_us(2);
        PcdAntennaOn();		//开天线
    }
}

/**
  * @brief  通过RC522和ISO14443卡通讯
  * @param[in]  Command 	RC522命令字
  * @param[in]  pInData		通过RC522发送到卡片的数据
  * @param[in]  InLenByte	发送数据的字节长度
  * @param[out] pOutData	接收到的卡片返回数据
  * @param[out] pOutLenBit	返回数据的位长度
  * @retval 状态值= MI_OK，成功
  */
char PcdComMF522(unsigned char Command,
                 unsigned char *pInData,
                 unsigned char InLenByte,
                 unsigned char *pOutData,
                 unsigned int  *pOutLenBit)
{
    char status = MI_ERR;
    unsigned char irqEn   = 0x00;
    unsigned char waitFor = 0x00;
    unsigned char lastBits;
    unsigned char n;
    unsigned int i;

    switch (Command)
    {
    case PCD_AUTHENT:	//Mifare认证
        irqEn   = 0x12;	//允许错误中断请求ErrIEn  允许空闲中断IdleIEn
        waitFor = 0x10;	//认证寻卡等待时候 查询空闲中断标志位
        break;
    case PCD_TRANSCEIVE://接收发送 发送接收
        irqEn   = 0x77;	//允许TxIEn RxIEn IdleIEn LoAlertIEn ErrIEn TimerIEn
        waitFor = 0x30;	//寻卡等待时候 查询接收中断标志位与 空闲中断标志位
        break;
    default:
        break;
    }

    WriteRawRC(ComIEnReg, irqEn|0x80);	//IRqInv置位管脚IRQ与Status1Reg的IRq位的值相反
    ClearBitMask(ComIrqReg, 0x80);		//Set1该位清零时，CommIRqReg的屏蔽位清零
    WriteRawRC(CommandReg, PCD_IDLE);	//写空闲命令
    SetBitMask(FIFOLevelReg, 0x80);		//置位FlushBuffer清除内部FIFO的读和写指针以及ErrReg的BufferOvfl标志位被清除

    for (i=0; i<InLenByte; i++)			//写数据进FIFOdata
        WriteRawRC(FIFODataReg, pInData[i]);

    WriteRawRC(CommandReg, Command);	//写命令

    if (Command == PCD_TRANSCEIVE)
        SetBitMask(BitFramingReg, 0x80);	//StartSend置位启动数据发送 该位与收发命令使用时才有效

    i = 1000;	//根据时钟频率调整，操作M1卡最大等待时间25ms
    do {
        n = ReadRawRC(ComIrqReg);
        i--;
    } while ((i!=0) && !(n&0x01) && !(n&waitFor));
    ClearBitMask(BitFramingReg, 0x80);	//清理允许StartSend位

//	LOG_PRINT("i=%d  ComIrqReg = %02x \r\n", i, n);

    if (i!=0)
    {
        if(!(ReadRawRC(ErrorReg)&0x1B))//读错误标志寄存器BufferOfI CollErr ParityErr ProtocolErr
        {
            status = MI_OK;
            if (n & irqEn & 0x01)	//是否发生定时器中断
                status = MI_NOTAGERR;

            if (Command == PCD_TRANSCEIVE)
            {
                n = ReadRawRC(FIFOLevelReg);				//读FIFO中保存的字节数
                lastBits = ReadRawRC(ControlReg) & 0x07;	//最后接收到得字节的有效位数

                if (lastBits)
                    *pOutLenBit = (n-1)*8 + lastBits;		//N个字节数减去1（最后一个字节）+最后一位的位数 读取到的数据总位数
                else
                    *pOutLenBit = n*8;						//最后接收到的字节整个字节有效

                if (n == 0)
                    n = 1;

                if (n > MAXRLEN)
                    n = MAXRLEN;

                for (i=0; i<n; i++)
                    pOutData[i] = ReadRawRC(FIFODataReg);
            }
        }
        else
            status = MI_ERR;

    }

    SetBitMask(ControlReg, 0x80);           // stop timer now
    WriteRawRC(CommandReg, PCD_IDLE);
    return status;
}

/**
  * @brief 寻卡
  * @param[in]  req_code，寻卡方式 = 0x52，寻感应区内所有符合14443A标准的卡；
                寻卡方式= 0x26，寻未进入休眠状态的卡
  * @param[out] pTagType，卡片类型代码
				= 0x4400，Mifare_UltraLight
				= 0x0400，Mifare_One(S50)
				= 0x0200，Mifare_One(S70)
				= 0x0800，Mifare_Pro(X))
				= 0x4403，Mifare_DESFire
  * @retval 状态值= MI_OK，成功
  */
char PcdRequest(unsigned char req_code,unsigned char *pTagType)
{
    char status;
    unsigned int  unLen;
    unsigned char ucComMF522Buf[MAXRLEN];

    ClearBitMask(Status2Reg, 0x08);		//清理指示MIFARECyptol单元接通以及所有卡的数据通信被加密的情况
    WriteRawRC(BitFramingReg, 0x07);	//发送的最后一个字节的七位
    SetBitMask(TxControlReg, 0x03);		//TX1,TX2管脚的输出信号传递经发送调制的13.56的能量载波信号

    ucComMF522Buf[0] = req_code;		//存入卡片命令字

    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 1, ucComMF522Buf, &unLen);	//寻卡

//	LOG_PRINT("\r\n>>%d >>%d >>%02x %02x\r\n", status, unLen, ucComMF522Buf[0], ucComMF522Buf[1]);
    if ((status == MI_OK) && (unLen == 0x10))	//寻卡成功返回卡类型
    {
        *pTagType     = ucComMF522Buf[0];
        *(pTagType+1) = ucComMF522Buf[1];
    }
    else
        status = MI_ERR;

    return status;
}

/**
  * @brief  防冲撞
  * @param[out] pSnr，卡片序列号，4字节
  * @retval 状态值= MI_OK，成功
  */
char PcdAnticoll(unsigned char *pSnr)
{
    char status;
    unsigned char i, snr_check=0;
    unsigned int  unLen;
    unsigned char ucComMF522Buf[MAXRLEN];


    ClearBitMask(Status2Reg, 0x08);	//清MFCryptol On位 只有成功执行MFAuthent命令后，该位才能置位
    WriteRawRC(BitFramingReg, 0x00);//清理寄存器 停止收发
    ClearBitMask(CollReg, 0x80);	//清ValuesAfterColl所有接收的位在冲突后被清除

    ucComMF522Buf[0] = PICC_ANTICOLL1;	//卡片防冲突命令
    ucComMF522Buf[1] = 0x20;

    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 2, ucComMF522Buf, &unLen);

    if (status == MI_OK)	//通信成功
    {
        for (i=0; i<4; i++)
        {
            *(pSnr+i)  = ucComMF522Buf[i];	//读出UID
            snr_check ^= ucComMF522Buf[i];
            //LOG_PRINT(">>%x\r\n", ucComMF522Buf[i]);
        }
        if (snr_check != ucComMF522Buf[i])
            status = MI_ERR;
    }

    SetBitMask(CollReg, 0x80);
    return status;
}

/**
  * @brief  用RC522计算CRC16
  * @param[in]  pIndata		计算CRC16的数组
  * @param[in]  len			计算CRC16的数组字节长度
  * @param[out] pOutData	存放计算结果存放的首地址
  * @retval 无
  */
void CalulateCRC(unsigned char *pIndata, unsigned char len, unsigned char *pOutData)
{
    unsigned char i, n;

    ClearBitMask(DivIrqReg, 0x04);
    WriteRawRC(CommandReg, PCD_IDLE);
    SetBitMask(FIFOLevelReg, 0x80);

    for (i=0; i<len; i++)
        WriteRawRC(FIFODataReg, *(pIndata+i));

    WriteRawRC(CommandReg, PCD_CALCCRC);

    i = 0xFF;
    do {
        n = ReadRawRC(DivIrqReg);
        i--;
    } while ((i!=0) && !(n&0x04));

    pOutData[0] = ReadRawRC(CRCResultRegL);
    pOutData[1] = ReadRawRC(CRCResultRegM);
}

/**
  * @brief  选定卡片
  * @param[in] pSnr，卡片序列号，4字节
  * @retval 状态值= MI_OK，成功
  */
char PcdSelect(unsigned char *pSnr)
{
    char status;
    unsigned char i;
    unsigned int  unLen;
    unsigned char ucComMF522Buf[MAXRLEN];

    ucComMF522Buf[0] = PICC_ANTICOLL1;
    ucComMF522Buf[1] = 0x70;
    ucComMF522Buf[6] = 0;
    for (i=0; i<4; i++)
    {
        ucComMF522Buf[i+2] = *(pSnr+i);
        ucComMF522Buf[6]  ^= *(pSnr+i);
    }
    //LOG_PRINT("check data is : %x\r\n", ucComMF522Buf[6]);
    CalulateCRC(ucComMF522Buf, 7, &ucComMF522Buf[7]);

    ClearBitMask(Status2Reg, 0x08);

    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 9, ucComMF522Buf, &unLen);

    if ((status == MI_OK) && (unLen == 0x18))
        status = MI_OK;
    else
        status = MI_ERR;

    return status;
}

/**
  * @brief  验证卡片密码
  * @param[in] 	auth_mode，密码验证模式= 0x60，验证A密钥，
				密码验证模式= 0x61，验证B密钥
  * @param[in]  addr，块地址
  * @param[in]  pKey，密码
  * @param[in]  pSnr，卡片序列号，4字节
  * @retval 状态值= MI_OK，成功
  */
char PcdAuthState(unsigned char auth_mode, unsigned char addr, unsigned char *pKey, unsigned char *pSnr)
{
    char status;
    unsigned int  unLen;
    unsigned char i, ucComMF522Buf[MAXRLEN];

    ucComMF522Buf[0] = auth_mode;
    ucComMF522Buf[1] = addr;
    for (i=0; i<6; i++)
        ucComMF522Buf[i+2] = *(pKey+i);

    for (i=0; i<6; i++)
        ucComMF522Buf[i+8] = *(pSnr+i);
//	memcpy(&ucComMF522Buf[2], pKey, 6);
//	memcpy(&ucComMF522Buf[8], pSnr, 4);

    status = PcdComMF522(PCD_AUTHENT, ucComMF522Buf, 12, ucComMF522Buf, &unLen);
    if ((status != MI_OK) || (!(ReadRawRC(Status2Reg) & 0x08)))
        status = MI_ERR;

    return status;
}

/**
  * @brief  命令卡片进入休眠状态
  * @retval 状态值= MI_OK，成功
  */
char PcdHalt(void)
{
    char status;
    unsigned int  unLen;
    unsigned char ucComMF522Buf[MAXRLEN];

    ucComMF522Buf[0] = PICC_HALT;
    ucComMF522Buf[1] = 0;
    CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);

    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);

    return status;
}

/**
  * @brief  读取M1卡一块数据
  * @param[in]  addr	块地址
  * @param[out] pData	读出的数据，16字节
  * @retval 状态值= MI_OK，成功
  */
char PcdRead(unsigned char addr, unsigned char *pData)
{
    char status;
    unsigned int  unLen;
    unsigned char i,ucComMF522Buf[MAXRLEN];

    ucComMF522Buf[0] = PICC_READ;
    ucComMF522Buf[1] = addr;
    CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);

    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);
    if ((status == MI_OK) && (unLen == 0x90))
//   {   memcpy(pData, ucComMF522Buf, 16);   }
    {
        for (i=0; i<16; i++)
            *(pData+i) = ucComMF522Buf[i];
    }
    else
        status = MI_ERR;

    return status;
}

/**
  * @brief  写数据到M1卡一块
  * @param[in]  addr，块地址
  * @param[in]  pData，写入的数据，16字节
  * @retval 状态值= MI_OK，成功
  */
char PcdWrite(unsigned char addr, unsigned char *pData)
{
    char status;
    unsigned int  unLen;
    unsigned char i, ucComMF522Buf[MAXRLEN];

    ucComMF522Buf[0] = PICC_WRITE;
    ucComMF522Buf[1] = addr;
    CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);

    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);

    if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
        status = MI_ERR;

    if (status == MI_OK)
    {
        //memcpy(ucComMF522Buf, pData, 16);
        for (i=0; i<16; i++)
            ucComMF522Buf[i] = *(pData+i);

        CalulateCRC(ucComMF522Buf,16,&ucComMF522Buf[16]);

        status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 18, ucComMF522Buf, &unLen);

        if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
            status = MI_ERR;
    }
    return status;
}

/**
  * @brief  扣款和充值
  * @param[in] dd_mode	命令字
					0xC0 = 扣款（减 Decrement）
					0xC1 = 充值（加 Increment）
  * @param[in] addr		钱包地址
  * @param[in] pValue	4字节增(减)值，低位在前
  * @retval 状态值= MI_OK，成功
  */
char PcdValue(unsigned char dd_mode, unsigned char addr, unsigned char *pValue)
{
    char status;
    unsigned int  unLen;
    unsigned char i,ucComMF522Buf[MAXRLEN];

    ucComMF522Buf[0] = dd_mode;
    ucComMF522Buf[1] = addr;
    CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);

    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);

    if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
    {
        status = MI_ERR;
    }

    if (status == MI_OK)
    {
        // memcpy(ucComMF522Buf, pValue, 4);
        for (i=0; i<16; i++)
        {
            ucComMF522Buf[i] = *(pValue+i);
        }
        CalulateCRC(ucComMF522Buf, 4, &ucComMF522Buf[4]);
        unLen = 0;
        status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 6, ucComMF522Buf, &unLen);
        if (status != MI_ERR)
        {
            status = MI_OK;
        }
    }

    if (status == MI_OK)
    {
        ucComMF522Buf[0] = PICC_TRANSFER;
        ucComMF522Buf[1] = addr;
        CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);

        status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);

        if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
        {
            status = MI_ERR;
        }
    }
    return status;
}

/**
  * @brief  备份钱包
  * @param[in] sourceaddr	源地址
  * @param[in] goaladdr		目标地址
  * @retval 状态值= MI_OK，成功
  */
char PcdBakValue(unsigned char sourceaddr, unsigned char goaladdr)
{
    char status;
    unsigned int  unLen;
    unsigned char ucComMF522Buf[MAXRLEN];

    ucComMF522Buf[0] = PICC_RESTORE;
    ucComMF522Buf[1] = sourceaddr;
    CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);

    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);

    if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
    {
        status = MI_ERR;
    }

    if (status == MI_OK)
    {
        ucComMF522Buf[0] = 0;
        ucComMF522Buf[1] = 0;
        ucComMF522Buf[2] = 0;
        ucComMF522Buf[3] = 0;
        CalulateCRC(ucComMF522Buf, 4, &ucComMF522Buf[4]);

        status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 6, ucComMF522Buf, &unLen);
        if (status != MI_ERR)
        {
            status = MI_OK;
        }
    }

    if (status != MI_OK)
    {
        return MI_ERR;
    }

    ucComMF522Buf[0] = PICC_TRANSFER;
    ucComMF522Buf[1] = goaladdr;

    CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);

    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);

    if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
    {
        status = MI_ERR;
    }

    return status;
}




/******************* (C) COPYRIGHT 2019 FiberHome *****END OF FILE****/

