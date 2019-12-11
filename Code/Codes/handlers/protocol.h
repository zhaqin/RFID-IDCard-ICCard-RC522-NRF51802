#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <stdint.h>

#define HTONS(x)  ((((x) & 0x00ff) << 8) | (((x) & 0xff00) >> 8))
#define NTOHS(x)  HTONS(x)
#define HTONL(x)  ((((x) & 0x000000ffUL) << 24) | \
                    (((x) & 0x0000ff00UL) << 8) | \
                    (((x) & 0x00ff0000UL) >> 8) | \
                    (((x) & 0xff000000UL) >> 24))

#define NTOHL(x)  HTONL(x)
#define BLE_HEAD  0xfbfa

#define READ_LOCK_INFO          1
#define DOOR_OPERATION          3
#define READ_KEY_BAT_INFO       6
#define READ_LOCK_DETAIL_INFO   8
#define APP_APPROVE_LOCK        20
#define LOCK_APPROVE_APP        21
#define MIDDLE_KEY              22
#define REAL_KEY                23

//uint16_t reverse16(uint16_t data){
//	return NTOHS(data);
//}
//uint32_t reverse32(uint32_t data){
//	return NTOHL(data);
//}
#pragma pack (1)
typedef struct _FRAME_TYPE{
	uint16_t head;
	uint8_t cmd;
	uint16_t data_len;
	uint8_t data[0];
    uint16_t check;
}FRAME_TYPE;

typedef struct _DIGITAL_KEY{
    uint8_t user_id[4];
    uint8_t lock_id[16];
    uint8_t approve_start_time[14];
    uint8_t approve_stop_time[14];
    uint8_t approve_time[14];
    uint8_t retain[2];
}DIGITAL_KEY;

typedef struct _BAT_INFO{
    uint8_t bat_quantity;
    uint8_t manufacturer;
    uint8_t version[2];
    uint8_t encrypt;
}BAT_INFO;
#pragma pack(0)
#endif
