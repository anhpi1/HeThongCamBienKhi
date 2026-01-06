#ifndef __RTC_H
#define __RTC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"// Thay đổi theo dòng chip STM32 bạn sử dụng
#include "stdio.h"
#include <stdint.h>

#define RTC_ADDRESS 0b1101000

typedef struct {
    uint8_t seconds;
    uint8_t minutes;  
    uint8_t hours;
    uint8_t day;
    uint8_t date; 
    uint8_t month;
    uint8_t year;
} RTC_TimeTypeDef;

extern RTC_TimeTypeDef rtcTime;
extern I2C_HandleTypeDef *RTC_hi2c;

int RTC_INIT(uint8_t s, uint8_t m, uint8_t h, uint8_t day, uint8_t date, uint8_t month, uint8_t year);
int RTC_read(RTC_TimeTypeDef *rtcTime);

#ifdef __cplusplus
}
#endif

#endif /* __RTC_H */