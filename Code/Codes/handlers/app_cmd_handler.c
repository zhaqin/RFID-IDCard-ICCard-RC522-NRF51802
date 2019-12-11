//app_cmd_handler.c
#include "des.h"
#include "SEGGER_RTT.h"
#include "app_cmd_handler.h"
#include "protocol.h"
#include "com_endian.h"

typedef struct _DEVICE_INFO{
    uint8_t lock_id[16];
    uint8_t manufacturer;
    uint8_t hard_version[2];
    uint8_t soft_version[2];
    uint8_t encrypt;
    uint8_t lock_install_state;
}DEVICE_INFO;
DEVICE_INFO device_info;
static const uint8_t root_key[8] = ROOT_KEY;
static const uint8_t ori_key[8] = ORI_KEY;
uint8_t middle_key[8] = {0};
uint8_t real_key[8] = {0};

uint8_t app_identify_lock(uint8_t* pdata,uint16_t len){
    if(pdata == NULL){
        return 1;
    }
    static uint8_t xor_para[8] = {0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,0x5a};
    One_DES_IV_Decode_Block(NULL, pdata, ori_key);
    Xor(pdata,xor_para,8);
    One_DES_IV_Encrypt_Block(NULL, pdata, ori_key);
    return 0;
}

uint8_t creat_middle_key(uint8_t* pdata,uint16_t len){
    if(pdata == NULL){
        return 1;
    }
    memset(middle_key,0,8);
    memcpy(middle_key,pdata,8);
    One_DES_IV_Encrypt_Block(NULL, middle_key, ori_key);
    return 0;
}

uint8_t get_real_key(uint8_t* pdata,uint16_t len){
    if(pdata == NULL){
        return 1;
    }
    memcpy(real_key,pdata,8);
    One_DES_IV_Decode_Block(NULL, real_key, middle_key);
    return 0;
}
uint8_t read_lock_info(uint8_t* pdata,uint16_t len){
    uint8_t* lock_info = NULL;
    lock_info = (uint8_t *)malloc(32);
    if(lock_info = NULL){
        return 1;
    }
    uint8_t lock_state,instal_state;                                   //need add
    uint8_t current_time[14] = {0};                                    //need add
    uint32_t offset = 0;
    memset(lock_info,0,32);
    memcpy(lock_info,device_info.lock_id,16);
    offset+=16;
    PIN_STORE_CHAR(lock_info,lock_state,offset);
    PIN_STORE_CHAR(lock_info,instal_state,offset);
    memcpy(lock_info+offset,current_time,14);
    offset+=14;
    uint8_t* pencrypt = lock_info;
    //encrypt
    for(int i=0;i<((offset+7)/8);i++){           //一次加密8字节，次数向上取整
        One_DES_IV_Encrypt_Block(NULL, pencrypt, root_key);
        pencrypt+=8;
    }
    bluetooth_send(READ_LOCK_INFO,lock_info,offset);
    free(lock_info);
}
uint8_t door_control(uint8_t* pdata,uint16_t len){
    SEGGER_RTT_printf(0,"door_control\r\n");
    buffer_print(pdata,len);
    uint8_t ret = drv_one_wire_open_lock();
    bluetooth_send(DOOR_OPERATION,&ret,1);
    return 0;
}
uint8_t read_key_battary_info(uint8_t* pdata,uint16_t len){
    BAT_INFO bat_info;
    memset(&bat_info,0,sizeof(BAT_INFO));     
    bat_info.manufacturer = device_info.manufacturer;
    bat_info.encrypt = 1;
    return bluetooth_send(READ_LOCK_DETAIL_INFO,(uint8_t*)&bat_info,sizeof(BAT_INFO));
}
uint8_t read_lock_detail_info(uint8_t* pdata,uint16_t len){
    uint8_t* detail_info = NULL;
    detail_info = (uint8_t *)malloc(22);
    if(detail_info == NULL){
        return 1;
    }
    uint32_t offset = 0;
    memset(detail_info,0,22);
    memcpy(detail_info,device_info.lock_id,16);
    offset+=16;
    PIN_STORE_CHAR(detail_info,device_info.manufacturer,offset);
    PIN_STORE_SHORT(detail_info,device_info.hard_version,offset);
    PIN_STORE_SHORT(detail_info,device_info.soft_version,offset);
    PIN_STORE_CHAR(detail_info,device_info.encrypt,offset);
    bluetooth_send(READ_LOCK_DETAIL_INFO,detail_info,22);
    free(detail_info);
}

void test_des(){
    int i=0; 
 
	unsigned char MyKey[8] = "86324168";           // 初始密钥 8字节*8
	unsigned char MyMessage[8] = "85264793";       // 初始明文 
	unsigned char IVBUFF[8] = {0};
	SEGGER_RTT_printf(0,"key_data:\n");  // 
	for (i = 0; i<8; i++)
	{
		SEGGER_RTT_printf(0,"0x%02X ", MyKey[i]);
	}
	SEGGER_RTT_printf(0,"\r\n");
 
	SEGGER_RTT_printf(0,"clear text:\n");  // 
	for (i = 0; i<8; i++)
	{
		SEGGER_RTT_printf(0,"0x%02X ", MyMessage[i]);
	}
	SEGGER_RTT_printf(0,"\r\n");
 
	One_DES_IV_Encrypt_Block(NULL, MyMessage, MyKey);
	SEGGER_RTT_printf(0,"secrte text:\n");  // 信息已加密
	for(i=0;i<8;i++)           
	{
		SEGGER_RTT_printf(0,"0x%02X ", MyMessage[i]);
	}
	SEGGER_RTT_printf(0,"\r\n");
	
	memset(IVBUFF,0,8);
	One_DES_IV_Decode_Block(NULL, MyMessage, MyKey);
	SEGGER_RTT_printf(0,"decode :\n");  // 信息已加密
	for (i = 0; i<8; i++)
	{
		SEGGER_RTT_printf(0,"0x%02X ", MyMessage[i]);
	}
	SEGGER_RTT_printf(0,"\r\n");
}
void app_cmd_handler_init(){
}