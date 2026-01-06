#ifndef __DS18B20_H
#define __DS18B20_H

#include "stm32f1xx_hal.h" // Thay đổi theo dòng STM32 bạn sử dụng
#include "stdio.h"
#include "stdint.h"


#define DS18B20_DEBUG 0
#define DS18B20_TIME_OUT_US 1000
#define DS18B20_TIM_BASE htim3  // Thay đổi theo Timer bạn sử dụng
#define DS18B20_SIGNAL_RESPONE_PRESENCE_MIN_US 60
#define DS18B20_SIGNAL_RESPONE_PRESENCE_MAX_US 240
#define DS18B20_GPIO_PORT GPIOA
#define DS18B20_PIN GPIO_PIN_1
#define DS18B20_SIGNAL_CYCLE_US 60
#define DS18B20_RESOLUTION 0b01111111 // 12 bit resolution
#define DS18B20_HOLD_LOW() HAL_GPIO_WritePin(DS18B20_GPIO_PORT, DS18B20_PIN, 0)
#define DS18B20_RELEASE() HAL_GPIO_WritePin(DS18B20_GPIO_PORT, DS18B20_PIN, 1)

void DS18B20_delay_us(uint16_t us);
void DS18B20_delay_ms(uint16_t ms);
void DS18B20_delay_s(uint16_t s);
void DS18B20_check_time(uint16_t start_time ,uint16_t end_time, uint16_t *result_time);
int DS18B20_reset(void);
void DS18B20_transmit_data(uint8_t data);
void DS18B20_receive_data(uint8_t *data);
int DS18B20_init(void);
int DS18B20_start_conversion(float *temperature);

extern TIM_HandleTypeDef DS18B20_TIM_BASE;

#endif /* __DS18B20_H */