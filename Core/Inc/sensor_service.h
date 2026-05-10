#ifndef SENSOR_SERVICE_H
#define SENSOR_SERVICE_H

#include <stdint.h>

void SensorService_Init(void);
uint16_t SensorService_ReadCoPpm(uint8_t *ok);
float SensorService_ReadTmp117Celsius(uint8_t *ok, float fallback_temp_c);

#endif
