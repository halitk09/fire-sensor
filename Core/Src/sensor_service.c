#include "sensor_service.h"

#include "main.h"
#include "app_config.h"
#include "stm32g4xx_hal_adc_ex.h"

#include <stdint.h>

#define TMP117_I2C_ADDR (0x48U << 1U)
#define TMP117_TEMP_REG 0x00U

extern ADC_HandleTypeDef hadc1;
extern I2C_HandleTypeDef hi2c1;

static uint16_t SensorService_ConvertRawToPpm(uint16_t adc_raw);
static uint16_t SensorService_LowPassU16(uint16_t prev, uint16_t sample, uint32_t alpha_num, uint32_t alpha_den);
static float SensorService_LowPassF32(float prev, float sample, float alpha);

void SensorService_Init(void)
{
  (void)HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
}

uint16_t SensorService_ReadCoPpm(uint8_t *ok)
{
  static uint16_t filtered_ppm = 0U;
  uint16_t sample_ppm;
  uint16_t adc_raw = 0U;

  if (ok != 0)
  {
    *ok = 0U;
  }

  if (HAL_ADC_Start(&hadc1) != HAL_OK)
  {
    return filtered_ppm;
  }

  if (HAL_ADC_PollForConversion(&hadc1, APP_CFG_ADC_POLL_TIMEOUT_MS) != HAL_OK)
  {
    (void)HAL_ADC_Stop(&hadc1);
    return filtered_ppm;
  }

  adc_raw = (uint16_t)HAL_ADC_GetValue(&hadc1);
  (void)HAL_ADC_Stop(&hadc1);

  if (ok != 0)
  {
    *ok = 1U;
  }

  sample_ppm = SensorService_ConvertRawToPpm(adc_raw);
  filtered_ppm = SensorService_LowPassU16(filtered_ppm, sample_ppm, APP_CFG_CO_EMA_ALPHA_NUM, APP_CFG_CO_EMA_ALPHA_DEN);
  return filtered_ppm;
}

float SensorService_ReadTmp117Celsius(uint8_t *ok, float fallback_temp_c)
{
  uint8_t raw[2];
  int16_t temp_raw;
  float temp_c;
  static float filtered_temp_c = 25.0f;

  if (HAL_I2C_Mem_Read(&hi2c1, TMP117_I2C_ADDR, TMP117_TEMP_REG, I2C_MEMADD_SIZE_8BIT, raw, 2U,
                       APP_CFG_I2C_MEM_TIMEOUT_MS) != HAL_OK)
  {
    if (ok != 0)
    {
      *ok = 0U;
    }
    return fallback_temp_c;
  }

  temp_raw = (int16_t)(((uint16_t)raw[0] << 8) | raw[1]);
  if (ok != 0)
  {
    *ok = 1U;
  }
  temp_c = (float)temp_raw * 0.0078125f;
  filtered_temp_c = SensorService_LowPassF32(filtered_temp_c, temp_c, 0.25f);
  return filtered_temp_c;
}

static uint16_t SensorService_ConvertRawToPpm(uint16_t adc_raw)
{
  float vout;
  float slope_ppm_per_v;
  float ppm_f;

  vout = ((float)adc_raw * APP_CFG_CO_ADC_VREF_V) / (float)APP_CFG_ADC_FULL_SCALE;
  slope_ppm_per_v = APP_CFG_CO_SEN0466_REF_PPM / (APP_CFG_CO_SEN0466_VOUT1_AT_REF_V - APP_CFG_CO_SEN0466_VOUT0_V);
  ppm_f = slope_ppm_per_v * (vout - APP_CFG_CO_SEN0466_VOUT0_V);

  if (ppm_f < 0.0f)
  {
    ppm_f = 0.0f;
  }
  if (ppm_f > APP_CFG_CO_SEN0466_MAX_PPM)
  {
    ppm_f = APP_CFG_CO_SEN0466_MAX_PPM;
  }
  return (uint16_t)(ppm_f + 0.5f);
}

static uint16_t SensorService_LowPassU16(uint16_t prev, uint16_t sample, uint32_t alpha_num, uint32_t alpha_den)
{
  if ((alpha_den == 0U) || (alpha_num > alpha_den))
  {
    return sample;
  }
  return (uint16_t)(((uint32_t)prev * (alpha_den - alpha_num) + (uint32_t)sample * alpha_num) / alpha_den);
}

static float SensorService_LowPassF32(float prev, float sample, float alpha)
{
  if ((alpha <= 0.0f) || (alpha >= 1.0f))
  {
    return sample;
  }
  return (prev * (1.0f - alpha)) + (sample * alpha);
}
