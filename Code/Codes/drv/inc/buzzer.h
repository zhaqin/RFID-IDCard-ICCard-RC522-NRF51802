#ifndef _BUZZER_H
#define _BUZZER_H

#include <stdbool.h>
#include <stdint.h>
#include "nrf.h"
#include "nrf_gpiote.h"
#include "nrf_gpio.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_timer.h"
#include "nrf_drv_gpiote.h"
#include "app_error.h"
#include "nrf_delay.h"
#include "drv_id.h"

#define BZ_EN	5

extern nrf_ppi_channel_t ppi_channe2;


void buzzer_init(void);
void buzzer_onoff(bool onoff);
void buzzer_no_delay(uint32_t ms);

#endif

