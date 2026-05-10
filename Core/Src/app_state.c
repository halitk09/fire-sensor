#include "app_state.h"
#include "app_config.h"

static AppState_t s_state = APP_STATE_NORMAL;
static uint8_t s_alarm_enter_cnt = 0U;
static uint8_t s_alarm_exit_cnt = 0U;

static uint8_t AppState_IsAlarm(uint16_t co_ppm, float temp_c);
static uint8_t AppState_IsWarning(uint16_t co_ppm, float temp_c);
static uint8_t AppState_IsAlarmExit(uint16_t co_ppm, float temp_c);

void AppState_Init(void)
{
  s_state = APP_STATE_NORMAL;
  s_alarm_enter_cnt = 0U;
  s_alarm_exit_cnt = 0U;
}

void AppState_ForceError(void)
{
  s_state = APP_STATE_ERROR;
}

AppState_t AppState_Get(void)
{
  return s_state;
}

void AppState_Evaluate(uint16_t co_ppm, float temp_c)
{
  uint8_t alarm_condition = AppState_IsAlarm(co_ppm, temp_c);
  uint8_t warning_condition = AppState_IsWarning(co_ppm, temp_c);
  uint8_t exit_alarm_condition = AppState_IsAlarmExit(co_ppm, temp_c);

  if (s_state == APP_STATE_ERROR)
  {
    return;
  }

  if (alarm_condition)
  {
    if (s_alarm_enter_cnt < 255U)
    {
      s_alarm_enter_cnt++;
    }
    s_alarm_exit_cnt = 0U;
    if (s_alarm_enter_cnt >= APP_CFG_ALARM_ENTER_CNT_MAX)
    {
      s_state = APP_STATE_ALARM;
    }
    return;
  }

  s_alarm_enter_cnt = 0U;

  switch (s_state)
  {
    case APP_STATE_ALARM:
      if (exit_alarm_condition != 0U)
      {
        if (s_alarm_exit_cnt < 255U)
        {
          s_alarm_exit_cnt++;
        }
        if (s_alarm_exit_cnt >= APP_CFG_ALARM_EXIT_CNT_MAX)
        {
          s_state = warning_condition ? APP_STATE_WARNING : APP_STATE_NORMAL;
          s_alarm_exit_cnt = 0U;
        }
      }
      else
      {
        s_alarm_exit_cnt = 0U;
      }
      return;
    default:
      s_alarm_exit_cnt = 0U;
      s_state = warning_condition ? APP_STATE_WARNING : APP_STATE_NORMAL;
      break;
  }
}

static uint8_t AppState_IsAlarm(uint16_t co_ppm, float temp_c)
{
  return ((co_ppm > APP_CFG_CO_ALARM_PPM) || (temp_c > APP_CFG_TEMP_ALARM_C)) ? 1U : 0U;
}

static uint8_t AppState_IsWarning(uint16_t co_ppm, float temp_c)
{
  return ((co_ppm > APP_CFG_CO_WARNING_PPM) || (temp_c > APP_CFG_TEMP_WARNING_C)) ? 1U : 0U;
}

static uint8_t AppState_IsAlarmExit(uint16_t co_ppm, float temp_c)
{
  return ((co_ppm < APP_CFG_CO_EXIT_PPM) && (temp_c < APP_CFG_TEMP_EXIT_C)) ? 1U : 0U;
}
