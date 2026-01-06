/* USER CODE BEGIN Header */
/**
 ******************************************************************************
  * @file    user_diskio.h
  * @brief   This file contains the common defines and functions prototypes for
  *          the user_diskio driver.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
 /* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USER_DISKIO_H
#define __USER_DISKIO_H

#ifdef __cplusplus
 extern "C" {
#endif

/* USER CODE BEGIN 0 */


/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include <stdio.h>
/* Exported constants --------------------------------------------------------*/
#define SD_DEBUG_ON 0
#define SD_CS_HIGH() HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, 1)
#define SD_CS_LOW() HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, 0)
#define SD_CMD0 0
#define SD_CMD8 8
#define SD_CMD58 58
#define SD_CMD55 55
#define SD_CMD41 41
#define SD_CMD16 16
#define SD_CMD17 17
#define SD_CMD18 18
#define SD_CMD12 12
#define SD_CMD24 24
#define SD_CMD25 25
#define SD_CMD9 9
#define SD_R1 1
#define SD_R1b 1
#define SD_R7 7
#define SD_R3 3
#define SD_DATA_TOKEN_ERROR 0xff
#define SD_POSITION_9 9
#define SD_POSITION_10 10
#define SD_POSITION_11 11
#define SD_POSITION_12 12
#define SD_POSITION_14 14
#define SD_POSITION_25 25
#define SD_POSITION_34 34
#define bool uint8_t
#define SD_WAIT(time) HAL_Delay(time)
#define SD_WAIT_SPI(time) HAL_Delay(time)
#define SD_quantity_mess 100
#define SD_MMC 0
#define SD_SDSC 1
#define SD_SDHC 2
#define SD_TIME_OUT_RESPONE_SD 1000
#define SD_TIME_OUT_SPI 100
#define SD_DATA_LEN 512
#define SD_BAUND_RATE SPI_BAUDRATEPRESCALER_4


 /* Exported types ------------------------------------------------------------*/

 typedef struct {
   	    uint32_t capacity_mb;
   	    uint32_t block_size;
   	    uint32_t block_count;
   	    uint32_t max_speed_hz;
   	} SD_info;

  typedef struct{
  	volatile bool is_spi_tx_ready;
  	volatile bool is_spi_rx_ready;
  	bool is_card_ready;
  	bool is_MMC;
  	bool is_SDSC;
  	bool is_SDHC;
  	bool is_time_out_spi;
  	uint8_t data[SD_DATA_LEN+4];
  	uint8_t error_mess[SD_quantity_mess];
  	SD_info info;
  }SD_handle;



/* Exported functions ------------------------------------------------------- */
 uint8_t SD_crc7(uint8_t cmd, uint32_t arg);
 bool SD_receive_command(SD_handle *mySD_handle, uint8_t typeResponse, uint8_t *response);
 bool SD_transmit_command(uint8_t cmd, uint32_t arg,SD_handle *mySD_handle);
 bool SD_receive_data(SD_handle *mySD_handle, uint32_t data_len,uint8_t *buff);
 bool SD_SPI_tx_multi(SD_handle *mySD_handle, uint16_t len);
 bool SD_SPI_tx_multi(SD_handle *mySD_handle, uint16_t len);
 void SD_push_error_mess(uint8_t error, SD_handle *mySD_handle );
 void SD_print_error_mess(SD_handle *mySD_handle);
 void SD_Check(SD_handle *mySD_handle);
 void SD_check_R7(const uint8_t *data, SD_handle *mySD_handle);
 void SD_check_R1(uint8_t data, SD_handle *mySD_handle);
 void SD_check_R3(const uint8_t *data, SD_handle *mySD_handle);
 void SD_INIT(SD_handle *mySD_handle);
 void SD_READ(SD_handle *mySD_handle,BYTE pdrv,BYTE *buff,DWORD sector,UINT count);
 void SD_check_SD_data_token_err(const uint8_t *data,SD_handle *mySD_handle);
 void SD_check_SD_data_respone_token(const uint8_t *data,SD_handle *mySD_handle);
bool SD_transmit_dummy();
uint16_t SD_crc16(const uint8_t *pData, size_t len);
void SD_Set_SPI_Speed(uint32_t prescaler);
void SD_read_CSD(const uint8_t *csd, SD_info *info);
bool SD_receive_CSD(SD_handle *mySD_handle);

extern SD_handle *mySD_handle;
extern SPI_HandleTypeDef hspi1;
extern Diskio_drvTypeDef  USER_Driver;

/* USER CODE END 0 */

#ifdef __cplusplus
}
#endif

#endif /* __USER_DISKIO_H */
