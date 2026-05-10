#ifndef BLE_TRANSPORT_H
#define BLE_TRANSPORT_H

#include "app_types.h"

#include <stdint.h>

typedef enum
{
  BLE_FAULT_NONE = 0,
  BLE_FAULT_AT_ERR,
  BLE_FAULT_TIMEOUT
} BleFaultKind_t;

void BleTransport_Init(void);
void BleTransport_Process(void);
void BleTransport_SendAdv(const char *device_id, AppState_t app_state, uint16_t co_ppm, float temp_c,
                          AppErrCode_t err_code);
uint8_t BleTransport_HasFatalError(void);
BleFaultKind_t BleTransport_GetFaultKind(void);

#endif
