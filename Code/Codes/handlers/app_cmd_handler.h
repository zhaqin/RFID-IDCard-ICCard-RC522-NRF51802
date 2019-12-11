//app_cmd_handler.h
#ifndef _APP_HANDLER_H
#define _APP_HANDLER_H

#define ROOT_KEY   "SDtt6789"
#define ORI_KEY    "TMJJ_234"

#include "nrf.h"


uint8_t read_lock_detail_info(uint8_t* pdata,uint16_t len);
uint8_t read_key_battary_info(uint8_t* pdata,uint16_t len);
uint8_t door_control(uint8_t* pdata,uint16_t len);
uint8_t read_lock_info(uint8_t* pdata,uint16_t len);
uint8_t app_identify_lock(uint8_t* pdata,uint16_t len);
uint8_t creat_middle_key(uint8_t* pdata,uint16_t len);
uint8_t get_real_key(uint8_t* pdata,uint16_t len);

#endif
