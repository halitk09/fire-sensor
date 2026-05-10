#ifndef APP_TYPES_H
#define APP_TYPES_H

typedef enum
{
  APP_STATE_NORMAL = 1,
  APP_STATE_WARNING = 2,
  APP_STATE_ALARM = 3,
  APP_STATE_ERROR = 4
} AppState_t;

typedef enum
{
  APP_ERR_NONE = 0,
  APP_ERR_BLE_AT = 1,
  APP_ERR_BLE_TIMEOUT = 2,
  APP_ERR_TMP117_I2C = 3,
  APP_ERR_CO_ADC = 4
} AppErrCode_t;

#endif
