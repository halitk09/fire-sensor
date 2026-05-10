#include "ble_transport.h"

#include "app_config.h"
#include "main.h"
#include "uart_rx.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef enum
{
  BLE_STATE_IDLE = 0,
  BLE_STATE_WAIT_RESPONSE,
  BLE_STATE_ERROR
} BleState_t;

typedef enum
{
  BLE_PHASE_INIT = 0,
  BLE_PHASE_SET_ADV_PARAM,
  BLE_PHASE_SET_ADV_DATA,
  BLE_PHASE_START_ADV
} BlePhase_t;

static BleState_t s_ble_state = BLE_STATE_IDLE;
static uint32_t s_wait_deadline_ms = 0UL;
static uint8_t s_retry_cnt = 0U;
static uint8_t s_fatal_error = 0U;
static BlePhase_t s_phase = BLE_PHASE_INIT;
static uint8_t s_adv_pending = 0U;
static char s_pending_device_id[16];
static AppState_t s_pending_app_state = APP_STATE_NORMAL;
static AppErrCode_t s_pending_err = APP_ERR_NONE;
static uint16_t s_pending_co_ppm = 0U;
static float s_pending_temp_c = 0.0f;
static BleFaultKind_t s_fault_kind = BLE_FAULT_NONE;

extern UART_HandleTypeDef huart1;

static uint8_t BleTransport_IsOkResponse(const char *line);
static uint8_t BleTransport_IsErrorResponse(const char *line);
static void BleTransport_HandleFault(uint8_t is_timeout);
static void BleTransport_UartSend(const char *msg);
static void BleTransport_SendInitCommand(void);
static void BleTransport_SendAdvParamCommand(void);
static uint8_t BleTransport_SendAdvDataCommand(void);
static void BleTransport_SendAdvStartCommand(void);
static void BleTransport_StartWait(void);
static uint8_t BleTransport_BuildAdvHex(const char *device_id, AppState_t app_state, AppErrCode_t err_code,
                                        uint16_t co_ppm, float temp_c, char *out_hex, uint16_t out_len);
static uint8_t BleTransport_NibbleToHex(uint8_t nibble);
static uint32_t BleTransport_DeviceIdU32(const char *device_id);

void BleTransport_Init(void)
{
  s_ble_state = BLE_STATE_IDLE;
  s_wait_deadline_ms = 0UL;
  s_retry_cnt = 0U;
  s_fatal_error = 0U;
  s_phase = BLE_PHASE_INIT;
  s_adv_pending = 0U;
  s_fault_kind = BLE_FAULT_NONE;

  UartRx_Init();
  BleTransport_SendInitCommand();
  BleTransport_StartWait();
}

void BleTransport_SendAdv(const char *device_id, AppState_t app_state, uint16_t co_ppm, float temp_c,
                          AppErrCode_t err_code)
{
  if ((device_id == 0) || s_fatal_error)
  {
    return;
  }

  (void)snprintf(s_pending_device_id, sizeof(s_pending_device_id), "%s", device_id);
  s_pending_app_state = app_state;
  s_pending_err = err_code;
  s_pending_co_ppm = co_ppm;
  s_pending_temp_c = temp_c;
  s_adv_pending = 1U;

  if ((s_ble_state == BLE_STATE_IDLE) && (s_phase == BLE_PHASE_SET_ADV_DATA))
  {
    if (BleTransport_SendAdvDataCommand())
    {
      BleTransport_StartWait();
    }
  }
}

void BleTransport_Process(void)
{
  char line[64];

  if (UartRx_GetLine(line, sizeof(line)))
  {
    if (BleTransport_IsOkResponse(line))
    {
      switch (s_phase)
      {
        case BLE_PHASE_INIT:
          s_phase = BLE_PHASE_SET_ADV_PARAM;
          s_ble_state = BLE_STATE_IDLE;
          BleTransport_SendAdvParamCommand();
          BleTransport_StartWait();
          break;
        case BLE_PHASE_SET_ADV_PARAM:
          s_phase = BLE_PHASE_SET_ADV_DATA;
          s_ble_state = BLE_STATE_IDLE;
          if (s_adv_pending != 0U)
          {
            if (BleTransport_SendAdvDataCommand())
            {
              BleTransport_StartWait();
            }
          }
          break;
        case BLE_PHASE_SET_ADV_DATA:
          s_phase = BLE_PHASE_START_ADV;
          s_ble_state = BLE_STATE_IDLE;
          BleTransport_SendAdvStartCommand();
          BleTransport_StartWait();
          break;
        case BLE_PHASE_START_ADV:
          s_phase = BLE_PHASE_SET_ADV_DATA;
          s_ble_state = BLE_STATE_IDLE;
          s_retry_cnt = 0U;
          s_adv_pending = 0U;
          break;
        default:
          break;
      }
    }
    else if (BleTransport_IsErrorResponse(line))
    {
      BleTransport_HandleFault(0U);
    }
  }

  if ((s_ble_state == BLE_STATE_WAIT_RESPONSE) &&
      ((int32_t)(HAL_GetTick() - s_wait_deadline_ms) >= 0))
  {
    BleTransport_HandleFault(1U);
  }
}

uint8_t BleTransport_HasFatalError(void)
{
  return s_fatal_error;
}

BleFaultKind_t BleTransport_GetFaultKind(void)
{
  if (s_fatal_error == 0U)
  {
    return BLE_FAULT_NONE;
  }
  return s_fault_kind;
}

static uint8_t BleTransport_IsOkResponse(const char *line)
{
  if (line == 0)
  {
    return 0U;
  }
  return (strcmp(line, "OK") == 0) ? 1U : 0U;
}

static uint8_t BleTransport_IsErrorResponse(const char *line)
{
  if (line == 0)
  {
    return 0U;
  }
  return (strcmp(line, "ERROR") == 0) ? 1U : 0U;
}

static void BleTransport_HandleFault(uint8_t is_timeout)
{
  if (s_retry_cnt < APP_CFG_BLE_MAX_RETRY)
  {
    s_retry_cnt++;
    s_ble_state = BLE_STATE_IDLE;
    switch (s_phase)
    {
      case BLE_PHASE_INIT:
        BleTransport_SendInitCommand();
        BleTransport_StartWait();
        break;
      case BLE_PHASE_SET_ADV_PARAM:
        BleTransport_SendAdvParamCommand();
        BleTransport_StartWait();
        break;
      case BLE_PHASE_SET_ADV_DATA:
        if (s_adv_pending != 0U)
        {
          if (BleTransport_SendAdvDataCommand())
          {
            BleTransport_StartWait();
          }
        }
        break;
      case BLE_PHASE_START_ADV:
        BleTransport_SendAdvStartCommand();
        BleTransport_StartWait();
        break;
      default:
        break;
    }
    return;
  }

  s_ble_state = BLE_STATE_ERROR;
  s_fatal_error = 1U;
  s_fault_kind = (is_timeout != 0U) ? BLE_FAULT_TIMEOUT : BLE_FAULT_AT_ERR;
}

static void BleTransport_UartSend(const char *msg)
{
  if (msg == 0)
  {
    return;
  }
  (void)HAL_UART_Transmit(&huart1, (uint8_t *)msg, (uint16_t)strlen(msg), 100U);
}

static void BleTransport_SendInitCommand(void)
{
  BleTransport_UartSend("AT+BLEINIT=2\r\n");
}

static void BleTransport_SendAdvParamCommand(void)
{
  BleTransport_UartSend("AT+BLEADVPARAM=50,50,0,0,7,0,,\r\n");
}

static uint8_t BleTransport_SendAdvDataCommand(void)
{
  char adv_hex[64];
  char cmd[96];
  int len;

  if (!BleTransport_BuildAdvHex(s_pending_device_id, s_pending_app_state, s_pending_err, s_pending_co_ppm,
                                 s_pending_temp_c, adv_hex, (uint16_t)sizeof(adv_hex)))
  {
    return 0U;
  }

  len = snprintf(cmd, sizeof(cmd), "AT+BLEADVDATA=\"%s\"\r\n", adv_hex);
  if ((len > 0) && ((size_t)len < sizeof(cmd)))
  {
    BleTransport_UartSend(cmd);
    return 1U;
  }
  return 0U;
}

static void BleTransport_SendAdvStartCommand(void)
{
  BleTransport_UartSend("AT+BLEADVSTART\r\n");
}

static void BleTransport_StartWait(void)
{
  s_ble_state = BLE_STATE_WAIT_RESPONSE;
  s_wait_deadline_ms = HAL_GetTick() + APP_CFG_BLE_RESPONSE_TIMEOUT_MS;
}

static uint8_t BleTransport_BuildAdvHex(const char *device_id, AppState_t app_state, AppErrCode_t err_code,
                                         uint16_t co_ppm, float temp_c, char *out_hex, uint16_t out_len)
{
  uint8_t ad[31];
  uint16_t idx = 0U;
  uint16_t i;
  int16_t temp_x10 = (int16_t)(temp_c * 10.0f);
  uint32_t dev_id;

  if ((out_hex == 0) || (out_len < 36U))
  {
    return 0U;
  }

  if (APP_CFG_DEVICE_ID_ADV_U32 != 0UL)
  {
    dev_id = (uint32_t)APP_CFG_DEVICE_ID_ADV_U32;
  }
  else
  {
    dev_id = BleTransport_DeviceIdU32(device_id);
  }

  ad[idx++] = 0x02U;
  ad[idx++] = 0x01U;
  ad[idx++] = 0x06U;

  ad[idx++] = 0x0DU;
  ad[idx++] = 0xFFU;
  ad[idx++] = 0xFFU;
  ad[idx++] = 0xFFU;
  ad[idx++] = (uint8_t)app_state;
  ad[idx++] = (uint8_t)err_code;
  ad[idx++] = (uint8_t)(dev_id & 0xFFU);
  ad[idx++] = (uint8_t)((dev_id >> 8U) & 0xFFU);
  ad[idx++] = (uint8_t)((dev_id >> 16U) & 0xFFU);
  ad[idx++] = (uint8_t)((dev_id >> 24U) & 0xFFU);
  ad[idx++] = (uint8_t)(co_ppm & 0xFFU);
  ad[idx++] = (uint8_t)((co_ppm >> 8U) & 0xFFU);
  ad[idx++] = (uint8_t)(temp_x10 & 0xFF);
  ad[idx++] = (uint8_t)((temp_x10 >> 8U) & 0xFF);

  if ((uint16_t)(idx * 2U + 1U) > out_len)
  {
    return 0U;
  }

  for (i = 0U; i < idx; i++)
  {
    out_hex[2U * i] = (char)BleTransport_NibbleToHex((uint8_t)((ad[i] >> 4U) & 0x0FU));
    out_hex[2U * i + 1U] = (char)BleTransport_NibbleToHex((uint8_t)(ad[i] & 0x0FU));
  }
  out_hex[2U * idx] = '\0';
  return 1U;
}

static uint8_t BleTransport_NibbleToHex(uint8_t nibble)
{
  if (nibble < 10U)
  {
    return (uint8_t)('0' + nibble);
  }
  return (uint8_t)('A' + (nibble - 10U));
}

static uint32_t BleTransport_DeviceIdU32(const char *device_id)
{
  uint32_t h = 2166136261UL;

  if (device_id == 0)
  {
    return 0UL;
  }

  while (*device_id != '\0')
  {
    h ^= (uint32_t)(uint8_t)(*device_id++);
    h *= 16777619UL;
  }
  return h;
}
