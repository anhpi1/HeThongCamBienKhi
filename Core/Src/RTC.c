#include "RTC.h"

#if RTC_INIT_ON
int RTC_INIT(uint8_t s, uint8_t m, uint8_t h, uint8_t day, uint8_t date, uint8_t month, uint8_t year){
    uint8_t data[1] = {0x00}; // Tắt SQW/OUT
    if(HAL_I2C_Mem_Write(RTC_hi2c, RTC_ADDRESS<<1, 0x07, I2C_MEMADD_SIZE_8BIT, data, 1, 100)!= HAL_OK) return 1; 

    // Ghi giờ/phút/giây với BCD inline
    uint8_t s_bcd = ((s / 10) << 4) | (s % 10);
    if(HAL_I2C_Mem_Write(RTC_hi2c, RTC_ADDRESS<<1, 0x00, I2C_MEMADD_SIZE_8BIT, &s_bcd, 1, 100)!= HAL_OK) return 2;
    uint8_t m_bcd = ((m / 10) << 4) | (m % 10);
    if(HAL_I2C_Mem_Write(RTC_hi2c, RTC_ADDRESS<<1, 0x01, I2C_MEMADD_SIZE_8BIT, &m_bcd, 1, 100)!= HAL_OK) return 3;

    uint8_t h_bcd = ((h / 10) << 4) | (h % 10);
    if(HAL_I2C_Mem_Write(RTC_hi2c, RTC_ADDRESS<<1, 0x02, I2C_MEMADD_SIZE_8BIT, &h_bcd, 1, 100)!= HAL_OK) return 4;

    // day không phải BCD, ghi trực tiếp
    if(HAL_I2C_Mem_Write(RTC_hi2c, RTC_ADDRESS<<1, 0x03, I2C_MEMADD_SIZE_8BIT, &day, 1, 100)!= HAL_OK) return 5;

    uint8_t date_bcd = ((date / 10) << 4) | (date % 10);
    if(HAL_I2C_Mem_Write(RTC_hi2c, RTC_ADDRESS<<1, 0x04, I2C_MEMADD_SIZE_8BIT, &date_bcd, 1, 100)!= HAL_OK) return 6;

    uint8_t month_bcd = ((month / 10) << 4) | (month % 10);
    if(HAL_I2C_Mem_Write(RTC_hi2c, RTC_ADDRESS<<1, 0x05, I2C_MEMADD_SIZE_8BIT, &month_bcd, 1, 100)!= HAL_OK) return 7;

    uint8_t year_bcd = ((year / 10) << 4) | (year % 10);
    if(HAL_I2C_Mem_Write(RTC_hi2c, RTC_ADDRESS<<1, 0x06, I2C_MEMADD_SIZE_8BIT, &year_bcd, 1, 100)!= HAL_OK) return 8;
    
    return 0;
}
#endif

int RTC_read(RTC_TimeTypeDef *rtcTime)
{
    /* USER CODE END WHILE */
    if(HAL_I2C_Mem_Read(RTC_hi2c, RTC_ADDRESS<<1, 0x00, I2C_MEMADD_SIZE_8BIT, &rtcTime->seconds, 1, 100) != HAL_OK)return 1;
    if(HAL_I2C_Mem_Read(RTC_hi2c, RTC_ADDRESS<<1, 0x01, I2C_MEMADD_SIZE_8BIT, &rtcTime->minutes, 1, 100) != HAL_OK)return 2;
    if(HAL_I2C_Mem_Read(RTC_hi2c, RTC_ADDRESS<<1, 0x02, I2C_MEMADD_SIZE_8BIT, &rtcTime->hours, 1, 100) != HAL_OK)return 3;
    if(HAL_I2C_Mem_Read(RTC_hi2c, RTC_ADDRESS<<1, 0x03, I2C_MEMADD_SIZE_8BIT, &rtcTime->day, 1, 100) != HAL_OK)return 4;
    if(HAL_I2C_Mem_Read(RTC_hi2c, RTC_ADDRESS<<1, 0x04, I2C_MEMADD_SIZE_8BIT, &rtcTime->date, 1, 100) != HAL_OK)return 5;
    if(HAL_I2C_Mem_Read(RTC_hi2c, RTC_ADDRESS<<1, 0x05, I2C_MEMADD_SIZE_8BIT, &rtcTime->month, 1, 100) != HAL_OK)return 6;
    if(HAL_I2C_Mem_Read(RTC_hi2c, RTC_ADDRESS<<1, 0x06, I2C_MEMADD_SIZE_8BIT, &rtcTime->year, 1, 100) != HAL_OK)return 7;
    rtcTime->seconds = ((rtcTime->seconds >> 4) * 10) + (rtcTime->seconds & 0x0F); 
    rtcTime->minutes = ((rtcTime->minutes >> 4) * 10) + (rtcTime->minutes & 0x0F);
    rtcTime->hours   = ((rtcTime->hours >> 4) * 10) + (rtcTime->hours & 0x0F);
    rtcTime->date    = ((rtcTime->date >> 4) * 10) + (rtcTime->date & 0x0F);
    rtcTime->month   = ((rtcTime->month >> 4) * 10) + (rtcTime->month & 0x0F);
    rtcTime->year    = ((rtcTime->year >> 4) * 10) + (rtcTime->year & 0x0F);
    rtcTime->day     = rtcTime->day & 0x07;  // day không phải BCD

    return 0;
}
