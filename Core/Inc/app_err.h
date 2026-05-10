#ifndef APP_ERR_H
#define APP_ERR_H

#include "app_types.h"

void AppErr_Clear(void);
void AppErr_Set(AppErrCode_t code);
AppErrCode_t AppErr_Get(void);

#endif
