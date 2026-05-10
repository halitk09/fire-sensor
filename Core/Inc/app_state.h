#ifndef APP_STATE_H
#define APP_STATE_H

#include "app_types.h"

#include <stdint.h>

void AppState_Init(void);

void AppState_ForceError(void);
void AppState_Evaluate(uint16_t co_ppm, float temp_c);

AppState_t AppState_Get(void);

#endif
