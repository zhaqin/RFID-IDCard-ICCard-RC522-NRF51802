#ifndef __DRV_ID_H
#define	__DRV_ID_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nrf.h"
#include "bsp.h"
#include "app_pwm.h"
#include "nrf_drv_ppi.h"
#include "log.h"
#include "nrf_drv_gpiote.h"
#include "buzzer.h"
#include "nrf_delay.h"
#include "usr_elock.h"
#include "nrf_drv_rtc.h"


#define	PWM_FRE					8L		//125KHZ
#define DUTY						50L

//#define ID_125K_PIN			17
//#define ID_OUT_PIN			6

#define DATA_SIZE				10

static nrf_ppi_channel_t ppi_channel, ppi_channe2;


typedef enum _ID_STATE
{
	NOT_CRAD = 0,
	DATA_HEAD,
	DATA_BIT,
	DATA_ECC,
	READ_OK,
}ID_STATE;	
typedef struct _ID_VAR
{
	uint8_t sta_temp;
	uint8_t tick;
	uint8_t data_cnt;
	uint8_t	bit_cnt;
	uint8_t byte;
	uint8_t data;
	ID_STATE data_state;
	bool sta_wait;
	uint32_t card_num;		// Ê®½øÖÆ¿¨ºÅ
}ID_VAR;

extern ID_VAR card;

static uint8_t card_data[DATA_SIZE] = {0};


void drv_id_init(void);
void drv_id_125K_onoff(bool onoff);
void drv_id_read(void);
void rtc_config(void);
void id_read_state_machine(uint8_t value);



#endif	//--#define	__DRV_ID_H


