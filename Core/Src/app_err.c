#include "app_err.h"

static AppErrCode_t s_code = APP_ERR_NONE;

void AppErr_Clear(void)
{
  s_code = APP_ERR_NONE;
}

void AppErr_Set(AppErrCode_t code)
{
  if ((s_code == APP_ERR_NONE) && (code != APP_ERR_NONE))
  {
    s_code = code;
  }
}

AppErrCode_t AppErr_Get(void)
{
  return s_code;
}
