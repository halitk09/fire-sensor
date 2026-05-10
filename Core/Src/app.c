#include "app.h"

#include "app_config.h"
#include "app_err.h"
#include "app_state.h"
#include "app_types.h"
#include "ble_transport.h"
#include "main.h"
#include "sensor_service.h"

#include <stdint.h>

static uint32_t s_last_sample_ms = 0UL;
static uint32_t s_last_adv_ms = 0UL;
static uint32_t s_last_ble_process_ms = 0UL;

static uint16_t s_co_ppm = 0U;
static float s_temp_c = 25.0f;
static uint8_t s_co_hal_fail_cnt = 0U;
static uint8_t s_tmp_hal_fail_cnt = 0U;

static void App_TaskBleProcess(void);
static void App_TaskSampleAndEvaluate(void);
static void App_TaskSendAdv(void);

void App_Init(void)
{
  uint32_t now = HAL_GetTick();

  AppErr_Clear();
  AppState_Init();
  SensorService_Init();
  BleTransport_Init();

  s_last_sample_ms = now;
  s_last_adv_ms = now;
  s_last_ble_process_ms = now;
  s_co_hal_fail_cnt = 0U;
  s_tmp_hal_fail_cnt = 0U;
}

void App_Run(void)
{
  uint32_t now = HAL_GetTick();

  if ((now - s_last_ble_process_ms) >= APP_CFG_BLE_PROCESS_PERIOD_MS)
  {
    s_last_ble_process_ms = now;
    App_TaskBleProcess();
  }

  if ((now - s_last_sample_ms) >= APP_CFG_SAMPLE_PERIOD_MS)
  {
    s_last_sample_ms = now;
    App_TaskSampleAndEvaluate();
  }

  if ((now - s_last_adv_ms) >= APP_CFG_BLE_ADV_UPDATE_PERIOD_MS)
  {
    s_last_adv_ms = now;
    App_TaskSendAdv();
  }
}

static void App_TaskBleProcess(void)
{
  BleTransport_Process();
  if (BleTransport_HasFatalError())
  {
    BleFaultKind_t fk = BleTransport_GetFaultKind();

    if (fk == BLE_FAULT_TIMEOUT)
    {
      AppErr_Set(APP_ERR_BLE_TIMEOUT);
    }
    else
    {
      AppErr_Set(APP_ERR_BLE_AT);
    }
    AppState_ForceError();
  }
}

static void App_TaskSampleAndEvaluate(void)
{
  uint8_t temp_ok = 0U;
  uint8_t co_ok = 0U;

  s_co_ppm = SensorService_ReadCoPpm(&co_ok);
  if (co_ok != 0U)
  {
    s_co_hal_fail_cnt = 0U;
  }
  else
  {
    if (s_co_hal_fail_cnt < 255U)
    {
      s_co_hal_fail_cnt++;
    }
    if (s_co_hal_fail_cnt >= APP_CFG_SENSOR_HAL_FAIL_CNT_MAX)
    {
      AppErr_Set(APP_ERR_CO_ADC);
      AppState_ForceError();
      return;
    }
  }

  s_temp_c = SensorService_ReadTmp117Celsius(&temp_ok, s_temp_c);
  if (temp_ok != 0U)
  {
    s_tmp_hal_fail_cnt = 0U;
  }
  else
  {
    if (s_tmp_hal_fail_cnt < 255U)
    {
      s_tmp_hal_fail_cnt++;
    }
    if (s_tmp_hal_fail_cnt >= APP_CFG_SENSOR_HAL_FAIL_CNT_MAX)
    {
      AppErr_Set(APP_ERR_TMP117_I2C);
      AppState_ForceError();
      return;
    }
  }

  if ((co_ok != 0U) && (temp_ok != 0U))
  {
    AppState_Evaluate(s_co_ppm, s_temp_c);
  }
}

static void App_TaskSendAdv(void)
{
  BleTransport_SendAdv(APP_CFG_DEVICE_ID, AppState_Get(), s_co_ppm, s_temp_c, AppErr_Get());
}
