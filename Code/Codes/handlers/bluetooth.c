//bluetooth.c
#include "SEGGER_RTT.h"
#include "ble_nus.h"
#include "protocol.h"
#include "app_fifo.h"
#include "app_cmd_handler.h"
#include "bluetooth.h"
#include "string.h"
#include "com_endian.h"

extern app_fifo_t key_ble_fifo;
extern uint8_t ble_buf_fifo[256];

ENV_BLE env_ble;
uint16_t bluetooth_check_sum(uint8_t* para,uint16_t len){
    uint16_t sum = 0;
    for(int i=2;i<len;i++){
        sum += *(para+i); 
    }
//    SEGGER_RTT_printf(0,"sum = %x\r\n",sum);
    return (~sum);
}

uint8_t bluetooth_send(uint8_t cmdid,uint8_t* pdata,uint16_t length){
    uint8_t* frame = NULL;
    uint16_t offset = 0;
    frame = (uint8_t*)malloc(length+7);
    if(frame == NULL){
		return 1;
	}
    memset(frame,0,length+ 7);
    PIN_STORE_SHORT(frame,BLE_HEAD,offset);
    PIN_STORE_CHAR(frame,cmdid,offset);
    uint16_t len = HTONS(length+2);
    memcpy(frame+offset,&len,2);
    offset+=2;    
    memcpy(frame+offset,pdata,length);
    offset +=length;
    uint16_t check = HTONS(bluetooth_check_sum(frame,length+3));
    PIN_STORE_SHORT(frame,check,offset);
//    buffer_print(frame,length+7);
    ble_nus_bytes_send(frame,length+7);      
    free(frame);
    return 0;
}
uint8_t temp[2] = {0};
int32_t ble_cmd_handler_reg(uint8_t cmd_id,BLE_HANDLER pf_handler){
	uint8_t i;
	for(i = 0;i < BLE_MAX_NUM_OF_CMD_HANDLER;i++){
		if(env_ble.cmd_ids[i] == 0){
			env_ble.cmd_ids[i] = cmd_id;
			env_ble.cmd_handlers[i] = pf_handler;
			return 0;
		}
	}
	return 1;
}
BLE_HANDLER ble_cmd_handler_get(uint8_t cmd_id){
	uint8_t i;
	for(i = 0;i < BLE_MAX_NUM_OF_CMD_HANDLER;i++){
		if(env_ble.cmd_ids[i] == cmd_id){
			return env_ble.cmd_handlers[i];
		}
	}
	return NULL;
}
void ble_rx_dispatch(){
    BLE_HANDLER pf_handler = NULL;
    pf_handler = ble_cmd_handler_get(env_ble.rx_cmd);
    if(pf_handler == NULL){
        SEGGER_RTT_printf(0,"pf_handler is NULL!\r\n");
        return;
    }
    if(pf_handler(env_ble.rx_data+5,env_ble.rx_length-2)){
        SEGGER_RTT_printf(0,"pf_handler return error!\r\n");
    }
}
void ble_rx_handler(uint8_t data){
    FRAME_TYPE* pframe;
    temp[0] = temp[1];
    temp[1] = data;
    if(env_ble.rx_state == 0){
        if((temp[0] == 0xfa)&&(temp[1] == 0xfb)){         //find head
            env_ble.rx_state = 1;
            env_ble.rx_position = 1;
        }
    }else if(env_ble.rx_state == 1){
        env_ble.rx_cmd = data;                           //find cmd
        env_ble.rx_position++;
        env_ble.rx_state = 2;
    }else if(env_ble.rx_state == 2){                      //find length
        env_ble.rx_position++;
        if(env_ble.rx_position == 4){
            env_ble.rx_length = (uint16_t)((temp[0]<<8)|(temp[1]));
            env_ble.rx_state = 3;
        }
    }else if(env_ble.rx_state == 3){                      //get data
        env_ble.rx_position++;
        env_ble.rx_data[env_ble.rx_position] = data;
        if((env_ble.rx_position > RX_DATA_MAXLEN)||(env_ble.rx_position>env_ble.rx_length+3)){
            ble_rx_dispatch();
            temp[0] = 0;
            temp[1] = 0;
            env_ble.rx_state = 0;
        }
        
    }
}
void bluetooth_init(){
    ble_cmd_handler_reg(APP_APPROVE_LOCK,app_identify_lock);
    ble_cmd_handler_reg(READ_LOCK_INFO,read_lock_info);
    ble_cmd_handler_reg(DOOR_OPERATION,door_control);
    ble_cmd_handler_reg(READ_KEY_BAT_INFO,read_key_battary_info);
    ble_cmd_handler_reg(READ_LOCK_DETAIL_INFO,read_lock_detail_info);    
    ble_cmd_handler_reg(MIDDLE_KEY,creat_middle_key);
    ble_cmd_handler_reg(REAL_KEY,get_real_key);
 }
