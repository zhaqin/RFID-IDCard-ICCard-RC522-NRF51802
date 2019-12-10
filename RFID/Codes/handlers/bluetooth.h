//bluetooth.h
#ifndef _BLUETOOTH_H
#define _BLUETOOTH_H

#define RX_DATA_MAXLEN 255
#define BLE_MAX_NUM_OF_CMD_HANDLER 10

#include "nrf.h"

typedef uint8_t (*BLE_HANDLER)(uint8_t *pdata,uint16_t len);

typedef struct _ENV_BLE{
    uint8_t rx_state;
    uint8_t rx_position;
    uint16_t rx_length;
    uint8_t rx_cmd;
    uint8_t rx_data[RX_DATA_MAXLEN];
    uint8_t cmd_ids[BLE_MAX_NUM_OF_CMD_HANDLER];
		BLE_HANDLER cmd_handlers[BLE_MAX_NUM_OF_CMD_HANDLER];
}ENV_BLE;











#endif
