#ifndef APP_CONFIG_H
#define APP_CONFIG_H
/*----------------------------------------------------------------------------
 * Identity (BLE adv)
 *----------------------------------------------------------------------------*/
#ifndef APP_CFG_DEVICE_ID
#define APP_CFG_DEVICE_ID "G474-01"
#endif

#ifndef APP_CFG_DEVICE_ID_ADV_U32
#define APP_CFG_DEVICE_ID_ADV_U32 0UL
#endif

/*----------------------------------------------------------------------------
 * Superloop timing (app.c)
 *----------------------------------------------------------------------------*/
#ifndef APP_CFG_SAMPLE_PERIOD_MS
#define APP_CFG_SAMPLE_PERIOD_MS 1000UL
#endif

#ifndef APP_CFG_BLE_ADV_UPDATE_PERIOD_MS
#define APP_CFG_BLE_ADV_UPDATE_PERIOD_MS 1000UL
#endif

#ifndef APP_CFG_BLE_PROCESS_PERIOD_MS
#define APP_CFG_BLE_PROCESS_PERIOD_MS 20UL
#endif

/*----------------------------------------------------------------------------
 * Alarm / warning thresholds (app_state.c)
 *----------------------------------------------------------------------------*/
#ifndef APP_CFG_CO_WARNING_PPM
#define APP_CFG_CO_WARNING_PPM 35U
#endif

#ifndef APP_CFG_CO_ALARM_PPM
#define APP_CFG_CO_ALARM_PPM 50U
#endif

#ifndef APP_CFG_CO_EXIT_PPM
#define APP_CFG_CO_EXIT_PPM 45U
#endif

#ifndef APP_CFG_TEMP_WARNING_C
#define APP_CFG_TEMP_WARNING_C 45.0f
#endif

#ifndef APP_CFG_TEMP_ALARM_C
#define APP_CFG_TEMP_ALARM_C 60.0f
#endif

#ifndef APP_CFG_TEMP_EXIT_C
#define APP_CFG_TEMP_EXIT_C 55.0f
#endif

#ifndef APP_CFG_ALARM_ENTER_CNT_MAX
#define APP_CFG_ALARM_ENTER_CNT_MAX 3U
#endif

#ifndef APP_CFG_ALARM_EXIT_CNT_MAX
#define APP_CFG_ALARM_EXIT_CNT_MAX 5U
#endif

#ifndef APP_CFG_SENSOR_HAL_FAIL_CNT_MAX
#define APP_CFG_SENSOR_HAL_FAIL_CNT_MAX 3U
#endif

/*----------------------------------------------------------------------------
 * BLE AT transport (ble_transport.c)
 *----------------------------------------------------------------------------*/
#ifndef APP_CFG_BLE_RESPONSE_TIMEOUT_MS
#define APP_CFG_BLE_RESPONSE_TIMEOUT_MS 800UL
#endif

#ifndef APP_CFG_BLE_MAX_RETRY
#define APP_CFG_BLE_MAX_RETRY 3U
#endif

/*----------------------------------------------------------------------------
 * TMP117 I2C mem read timeout (ms)
 *----------------------------------------------------------------------------*/
#ifndef APP_CFG_I2C_MEM_TIMEOUT_MS
#define APP_CFG_I2C_MEM_TIMEOUT_MS 5U
#endif

/*----------------------------------------------------------------------------
 * CO channel: ADC
 *----------------------------------------------------------------------------*/
#ifndef APP_CFG_CO_PPM_FULL_SCALE
#define APP_CFG_CO_PPM_FULL_SCALE 1000UL
#endif

#ifndef APP_CFG_ADC_FULL_SCALE
#define APP_CFG_ADC_FULL_SCALE 4095UL
#endif

#ifndef APP_CFG_ADC_POLL_TIMEOUT_MS
#define APP_CFG_ADC_POLL_TIMEOUT_MS 10U
#endif

/*----------------------------------------------------------------------------
 * CO smoothing — EMA: new = (prev*(den-num) + sample*num) / den (sensor_service.c)
 *----------------------------------------------------------------------------*/
#ifndef APP_CFG_CO_EMA_ALPHA_NUM
#define APP_CFG_CO_EMA_ALPHA_NUM 1UL
#endif

#ifndef APP_CFG_CO_EMA_ALPHA_DEN
#define APP_CFG_CO_EMA_ALPHA_DEN 4UL
#endif

/*----------------------------------------------------------------------------
 * CO channel: SEN0466 voltage model
 *----------------------------------------------------------------------------*/
#ifndef APP_CFG_CO_ADC_VREF_V
#define APP_CFG_CO_ADC_VREF_V 3.3f
#endif

#ifndef APP_CFG_CO_SEN0466_VOUT0_V
#define APP_CFG_CO_SEN0466_VOUT0_V 0.6f
#endif

#ifndef APP_CFG_CO_SEN0466_VOUT1_AT_REF_V
#define APP_CFG_CO_SEN0466_VOUT1_AT_REF_V 0.9f
#endif

#ifndef APP_CFG_CO_SEN0466_REF_PPM
#define APP_CFG_CO_SEN0466_REF_PPM 200.0f
#endif

#ifndef APP_CFG_CO_SEN0466_MAX_PPM
#define APP_CFG_CO_SEN0466_MAX_PPM 1000.0f
#endif

#endif
