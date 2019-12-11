#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "ble_hci.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "softdevice_handler.h"
#include "app_timer.h"
#include "app_button.h"
#include "ble_nus.h"
#include "app_util_platform.h"
#include "bsp.h"
#include "bsp_btn_ble.h"
#include "nrf_delay.h"
#include "bsp_key.h"
#include "bsp_led.h"
#include "bsp_timer.h"
#include "buzzer.h"
#include "SEGGER_RTT.h"
#include "protocol.h"
#include "app_fifo.h"
#include "drv_rtc.h"
#include "log.h"
#include "drv_id.h"
#include "usr_elock.h"
#include "app_timer.h"
#include "drv_rc522.h"
#include "nrf_drv_config.h"
#include "drv_flash.h"
#include "drv_common.h"
#include "usr_elock.h"


extern ID_VAR card;

#define IS_SRVC_CHANGED_CHARACT_PRESENT 0                                           /**< Include the service_changed characteristic. If not enabled, the server's database cannot be changed for the lifetime of the device. */

const char DEVICE_NAME[17] = "zhang001";                               /**< Name of device. Will be included in the advertising data. */
#define NUS_SERVICE_UUID_TYPE           BLE_UUID_TYPE_VENDOR_BEGIN                  /**< UUID type for the Nordic UART Service (vendor specific). */
//#define NUS_SERVICE_UUID_TYPE                                          BLE_UUID_TYPE_BLE
//#define APP_ADV_INTERVAL                64                                          /**< The advertising interval (in units of 0.625 ms. This value corresponds to 40 ms). */
#define APP_ADV_INTERVAL                MSEC_TO_UNITS(100, UNIT_0_625_MS)
#define APP_ADV_TIMEOUT_IN_SECONDS      0                                         /**< The advertising timeout (in units of seconds). */

#define APP_TIMER_PRESCALER             0                                           /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_OP_QUEUE_SIZE         20                                          /**< Size of timer operation queues. */

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(20, UNIT_1_25_MS)             /**< Minimum acceptable connection interval (20 ms), Connection interval uses 1.25 ms units. */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(75, UNIT_1_25_MS)             /**< Maximum acceptable connection interval (75 ms), Connection interval uses 1.25 ms units. */
//#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(10, UNIT_1_25_MS)
//#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(300, UNIT_1_25_MS)
#define SLAVE_LATENCY                   0                                           /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)             /**< Connection supervisory timeout (4 seconds), Supervision Timeout uses 10 ms units. */
#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000, APP_TIMER_PRESCALER)  /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000, APP_TIMER_PRESCALER) /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

#define START_STRING                    "Start...\n"                                /**< The string that will be sent over the UART when the application starts. */

#define DEAD_BEEF                       0xDEADBEEF                                  /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define CHECK_STATE()		drv_lock_status()


app_fifo_t key_ble_fifo;
uint8_t ble_buf_fifo[64];
static ble_nus_t                        m_nus;                                      /**< Structure to identify the Nordic UART Service. */
static uint16_t                         m_conn_handle = BLE_CONN_HANDLE_INVALID;    /**< Handle of the current connection. */

static ble_uuid_t                       m_adv_uuids[] = {{BLE_UUID_NUS_SERVICE, NUS_SERVICE_UUID_TYPE}};  /**< Universally unique service identifier. */
static uint8_t ble_receive = 0;


void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    LOG_INF("error: %s %d", p_file_name, line_num);
}


/**@brief Function for the GAP initialization.
 *
 * @details This function will set up all the necessary GAP (Generic Access Profile) parameters of
 *          the device. It also sets the permissions and appearance.
 */
static void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;
    ble_gap_addr_t addr;
    sd_ble_gap_address_get(&addr);
//    buffer_print((uint8_t*)&addr,7);
//		sprintf(DEVICE_NAME,"%s%x%x%x%x","SDTT-0100",addr.addr[3],addr.addr[2],addr.addr[1],addr.addr[0]);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);//open link

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *) DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}
/**@brief Function for handling the data from the Nordic UART Service.
 *
 * @details This function will process the data received from the Nordic UART BLE Service and send
 *          it to the UART module.
 *
 * @param[in] p_nus    Nordic UART Service structure.
 * @param[in] p_data   Data to be send to UART module.
 * @param[in] length   Length of the data.
 */
/**@snippet [Handling the data received over BLE] */

static void nus_data_handler(ble_nus_t * p_nus, uint8_t * p_data, uint16_t length)
{
    for (uint32_t i = 0; i < length; i++)
    {
        app_fifo_put(&key_ble_fifo, p_data[i]);
    }
//    app_fifo_write(&key_ble_fifo,p_data,length);

    ble_receive = 1;
}
/**@snippet [Handling the data received over BLE] */


/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
    uint32_t       err_code;
    ble_nus_init_t nus_init;

    memset(&nus_init, 0, sizeof(nus_init));

    nus_init.data_handler = nus_data_handler;

    err_code = ble_nus_init(&m_nus, &nus_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling an event from the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module
 *          which are passed to the application.
 *
 * @note All this function does is to disconnect. This could have been done by simply setting
 *       the disconnect_on_fail config parameter, but instead we use the event handler
 *       mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    uint32_t err_code;

    if(p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for handling errors from the Connection Parameters module.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    uint32_t               err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
static void sleep_mode_enter(void)
{
    uint32_t err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    // Prepare wakeup buttons.
    err_code = bsp_btn_ble_sleep_mode_prepare();
    APP_ERROR_CHECK(err_code);

    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    uint32_t err_code;

    switch (ble_adv_evt)
    {
    case BLE_ADV_EVT_FAST:
        err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
        APP_ERROR_CHECK(err_code);
        break;
    case BLE_ADV_EVT_IDLE:
        sleep_mode_enter();
        break;
    default:
        break;
    }
}


/**@brief Function for the Application's S110 SoftDevice event handler.
 *
 * @param[in] p_ble_evt S110 SoftDevice event.
 */
static uint8_t ble_conn_state;
static void on_ble_evt(ble_evt_t * p_ble_evt)
{
    uint32_t                         err_code;
    switch (p_ble_evt->header.evt_id)
    {
    case BLE_GAP_EVT_CONNECTED:
        m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
        ble_conn_state = 1;
        LOG_PRINT("ble connect...\r\n");
        break;

    case BLE_GAP_EVT_DISCONNECTED:
        m_conn_handle = BLE_CONN_HANDLE_INVALID;
        ble_conn_state = 0;
        LOG_PRINT("ble disconnect...\r\n");
        break;

    case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
        // Pairing not supported
        err_code = sd_ble_gap_sec_params_reply(m_conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
        APP_ERROR_CHECK(err_code);
        break;

    case BLE_GATTS_EVT_SYS_ATTR_MISSING:
        // No system attributes have been stored.
        err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
        APP_ERROR_CHECK(err_code);
        break;

    default:
        // No implementation needed.
        break;
    }
}


/**@brief Function for dispatching a S110 SoftDevice event to all modules with a S110 SoftDevice
 *        event handler.
 *
 * @details This function is called from the S110 SoftDevice event interrupt handler after a S110
 *          SoftDevice event has been received.
 *
 * @param[in] p_ble_evt  S110 SoftDevice event.
 */
static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
    ble_conn_params_on_ble_evt(p_ble_evt);
    ble_nus_on_ble_evt(&m_nus, p_ble_evt);
    on_ble_evt(p_ble_evt);
    ble_advertising_on_ble_evt(p_ble_evt);

//    bsp_btn_ble_on_ble_evt(p_ble_evt);

}

/**@brief 系统事件处理函数
 */
static void sys_evt_dispatch(uint32_t sys_evt)
{
    fs_sys_event_handler(sys_evt);
}


/**@brief Function for the S110 SoftDevice initialization.
 *
 * @details This function initializes the S110 SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    uint32_t err_code;

    // Initialize SoftDevice.
    SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_XTAL_20_PPM, NULL);

    // Enable BLE stack.
    ble_enable_params_t ble_enable_params;
    memset(&ble_enable_params, 0, sizeof(ble_enable_params));
#if (defined(S130) || defined(S132))
    ble_enable_params.gatts_enable_params.attr_tab_size   = BLE_GATTS_ATTR_TAB_SIZE_DEFAULT;
#endif
    ble_enable_params.gatts_enable_params.service_changed = IS_SRVC_CHANGED_CHARACT_PRESENT;
    err_code = sd_ble_enable(&ble_enable_params);
    APP_ERROR_CHECK(err_code);

    // Subscribe for BLE events.
    err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
    APP_ERROR_CHECK(err_code);

    // Subscribe for SYS events.
    err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling events from the BSP module.
 *
 * @param[in]   event   Event generated by button press.
 */
void bsp_event_handler(bsp_event_t event)
{
    uint32_t err_code;
    switch (event)
    {
    case BSP_EVENT_SLEEP:
        sleep_mode_enter();
        break;

    case BSP_EVENT_DISCONNECT:
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        if (err_code != NRF_ERROR_INVALID_STATE)
        {
            APP_ERROR_CHECK(err_code);
        }
        break;

    case BSP_EVENT_WHITELIST_OFF:
        err_code = ble_advertising_restart_without_whitelist();
        if (err_code != NRF_ERROR_INVALID_STATE)
        {
            APP_ERROR_CHECK(err_code);
        }
        break;

    default:
        break;
    }
}


/**@brief   Function for handling app_uart events.
 *
 * @details This function will receive a single character from the app_uart module and append it to
 *          a string. The string will be be sent over BLE when the last character received was a
 *          'new line' i.e '\n' (hex 0x0D) or if the string has reached a length of
 *          @ref NUS_MAX_DATA_LENGTH.
 */
/**@snippet [Handling the data received over UART] */
void ble_nus_bytes_send(uint8_t *pdata,uint16_t length) {
//	 ble_nus_string_send(&m_nus, pdata, length);
    uint32_t       err_code,pos;
    for(pos = 0; pos < length;) {
        if((pos + 20) <= length) {
            err_code = ble_nus_string_send(&m_nus, pdata + pos, 20);
        } else {
            err_code = ble_nus_string_send(&m_nus, pdata + pos, length - pos);
        }
        if(err_code != NRF_ERROR_INVALID_STATE) {
            APP_ERROR_CHECK(err_code);
        }
        pos += 20;
    }
}


/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
{
    uint32_t      err_code;
    ble_advdata_t advdata;
    ble_advdata_t scanrsp;

    // Build advertising data struct to pass into @ref ble_advertising_init.
    memset(&advdata, 0, sizeof(advdata));
    advdata.name_type          = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance = false;
    advdata.flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

    memset(&scanrsp, 0, sizeof(scanrsp));
    scanrsp.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    scanrsp.uuids_complete.p_uuids  = m_adv_uuids;

    ble_adv_modes_config_t options = {0};
    options.ble_adv_fast_enabled  = BLE_ADV_FAST_ENABLED;
    options.ble_adv_fast_interval = APP_ADV_INTERVAL;
    options.ble_adv_fast_timeout  = APP_ADV_TIMEOUT_IN_SECONDS;

    err_code = ble_advertising_init(&advdata, &scanrsp, &options, on_adv_evt, NULL);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for placing the application in low power state while waiting for events.
 */

static void power_manage(void)
{
    uint32_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}

static void ble_rx_check() {
    uint8_t data,temp;
    if(ble_receive) {
        if(app_fifo_get(&key_ble_fifo,&data) == NRF_SUCCESS) {
//						LOG_PRINT("%x\r\n", data);
            switch(data) {
            case 0xAF:
                drv_lock_led_set(BSP_INDICATE_ELOCK_STATE_ON, 0);
                break;
            case 0xA0:
                drv_lock_led_set(BSP_INDICATE_ELOCK_STATE_OFF, 0);
                break;
            case 0xBF:
                buzzer_onoff(1);
                break;
            case 0xB0:
                buzzer_onoff(0);
                break;
            case 0xAB:
                drv_id_read();
                break;
            case 0xCC:
                temp = CHECK_STATE();
                ble_nus_bytes_send(&temp, 1);
                break;
            case 0xD0:
            {
                g_elock_info.card_handle_state = CARD_STATE_VERIFY;
                LOG_PRINT("CARD_STATE_VERIFY set...\r\n");
            }
            break;
            case 0xD1:
            {
                g_elock_info.card_handle_state = CARD_STATE_ADD;
                LOG_PRINT("CARD_STATE_ADD set...\r\n");
            }
            break;
            case 0xD2:
            {
                g_elock_info.card_handle_state = CARD_STATE_DELETE;
                LOG_PRINT("CARD_STATE_DELETE set...\r\n");
            }
            break;
            case 0xD3:
            {
                elock_card_ops(CARD_NUM_FLASH_PUT, NULL, NULL);
            }
            break;
            default:
                break;
            }
//            ble_rx_handler(data);
        }
        ble_receive = 0;
    }
}

/**@brief Application main function.
 */
#define IC_UID_COPY         0

int main(void)
{
    uint32_t err_code;

    //start hfclk
    NRF_CLOCK->TASKS_HFCLKSTART = 1;
    while(NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);

    // Initialize.
    SEGGER_RTT_Init();
    nrf_drv_ppi_init();
    nrf_drv_gpiote_init();

    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, false);

    if(app_fifo_init(&key_ble_fifo, ble_buf_fifo, sizeof(ble_buf_fifo)))	LOG_ERR("ble fifo init fail");


    ble_stack_init();
    gap_params_init();
    services_init();

    advertising_init();
    conn_params_init();
    err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
    if(err_code != NRF_SUCCESS)	LOG_ERR("ble advertising start fail");

    buzzer_init();

    drv_id_init();			// ID读卡初始化
    rc522_enable_init();	// IC读卡初始化

    elock_card_init();		// 门禁卡处理初始化

    LOG_PRINT("init complete !\r\n");

    // Enter main loop.
    for (;;)
    {
        power_manage();

        if(ble_conn_state)
        {
            ble_rx_check();		//蓝牙通信
        }
		
        rc522_read();			//RC522寻卡
        elock_card_process();	//门禁卡读卡处理

    }
}


/**
 * @} app_elock
 */
