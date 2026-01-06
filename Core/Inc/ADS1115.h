#ifndef __ADS1115_H
#define __ADS1115_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "math.h"
#include "stdio.h"

typedef struct{
  uint8_t DR;
  float Rs_R0;
  float R0_RL;
} ADS1115_channel;

typedef struct{
  ADS1115_channel A0;
  ADS1115_channel A1;
  ADS1115_channel A2;
  ADS1115_channel A3;
} ADS1115_device;

typedef struct{
	uint8_t data[2];
  float vontage_ref;
  ADS1115_device GND;
  ADS1115_device VCC;
}ADS1115_handle;

#define ADS1115_ADRR_ADS1115_GND 0b1001000
#define ADS1115_ADRR_ADS1115_VCC 0b1001001
#define ADS1115_ARR_REG_CONFIG 0b01
#define ADS1115_ARR_REG_CONVERSION 0b00
#define ADS1115_A0 0b100
#define ADS1115_A1 0b101
#define ADS1115_A2 0b110
#define ADS1115_A3 0b111
#define ADS1115_DEBUG 0
#define ADS1115_ENABLE_ADC 1

extern ADS1115_handle my_ADS1115_1;
extern I2C_HandleTypeDef *ADS1115_hi2c;

#if ADS1115_ENABLE_ADC
  extern ADC_HandleTypeDef *ADS1115_hadc;
#endif


#if ADS1115_ENABLE_ADC
  int ADS1115_adc_read_once(uint16_t *value);
#endif

void ADS1115_wait(uint8_t DR);
int ADS1115_decode_device_address(uint8_t adrrDevice, ADS1115_device **device);
int  ADS1115_decode_channel_address(ADS1115_device *device, uint8_t channel, ADS1115_channel **ch);
int ADS1115_getADC(ADS1115_handle *my_ADS1115,uint8_t adrrDevice, uint8_t channel);
int ADS1115_caculator_normal_probability_distribution(float do_tin_cay, float *resurt);
int ADS1115_get_mean_and_varadiance(float *mean, float *variance, float *arr, uint16_t length);  
int ADS1115_get_num_sample_required(uint16_t *number_of_samples, float *arr, uint16_t length, float confidence_level, float error_margin);
int ADS1115_init(ADS1115_handle *my_ADS1115, uint8_t adrrDevice, uint8_t channel); 
int ADS1115_read_r0_rl(ADS1115_handle *my_ADS1115, uint8_t adrrDevice, uint8_t channel);
#ifdef __cplusplus
}
#endif

#endif /* __ADS1115_H */