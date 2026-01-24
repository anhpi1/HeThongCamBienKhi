/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "DHT22.h"
#include "stm32f103xb.h"
#include "stm32f1xx_hal.h"
#include "string.h"
#include "RTC.h"
#include "ADS1115.h"
#include "DS18B20.h"
#include "pid.h"
#include <math.h>
#include <sys/_intsup.h>
// Ensure ADS1115.c is compiled and linked with your project

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */



/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */


/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_rx;
DMA_HandleTypeDef hdma_spi1_tx;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
DMA_HandleTypeDef hdma_tim2_ch1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */


my_struct_data_captrure my_variable_capture_buf = {0};
SD_handle mySD_handle1 = {0};
SD_handle *mySD_handle = &mySD_handle1;

RTC_TimeTypeDef rtcTime;
I2C_HandleTypeDef *RTC_hi2c = &hi2c1;

ADS1115_handle my_ADS1115_1 ={0};
I2C_HandleTypeDef *ADS1115_hi2c = &hi2c1;
ADC_HandleTypeDef *ADS1115_hadc = &hadc1;


FRESULT fr;
FATFS fs;
FIL file;
UINT bw;

PID_TypeDef PIDHeat;
double InputHeat, OutputHeat, SetpointHeat;

PID_TypeDef PIDHum;
double InputHum, OutputHum, SetpointHum;

uint8_t rx;

float temperature = 0.0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM1_Init(void);
/* USER CODE BEGIN PFP */
void wait_to_start_mq(uint32_t wait_time_s);

void TASK_data_sensor(void);




/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int _write(int file, char *ptr, int len)
{
     HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
     return len;
}

static inline char* append_str(char *p, const char *s)
{
    while (*s) *p++ = *s++;
    return p;
}

static inline char* append_2d(char *p, int v)
{
    *p++ = '0' + (v / 10);
    *p++ = '0' + (v % 10);
    return p;
}


void float_to_str_2int(float v, char *buf)
{
    if (v < 0) v = 0;

    int ip = (int)v;
    int fp = (int)((v - ip) * 100000.0f + 0.5f);

    /* xử lý tràn do làm tròn */
    if (fp >= 100000) {
        fp = 0;
        ip++;
    }

    /* giới hạn 2 chữ số nguyên */
    if (ip > 99) ip = 99;

    /* XX */
    buf[0] = '0' + (ip / 10);
    buf[1] = '0' + (ip % 10);

    /* . */
    buf[2] = '.';

    /* YYYYY */
    buf[7] = '0' + (fp % 10); fp /= 10;
    buf[6] = '0' + (fp % 10); fp /= 10;
    buf[5] = '0' + (fp % 10); fp /= 10;
    buf[4] = '0' + (fp % 10); fp /= 10;
    buf[3] = '0' + (fp % 10);

    buf[8] = '\0';
}



/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_USART2_UART_Init();
  MX_FATFS_Init();
  MX_ADC1_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
  wait_to_start_mq(1);// đợi 5 phút cho các task khác khởi động xong trước khi mount sd card
  HAL_ADCEx_Calibration_Start(&hadc1);

  fr = f_mount(&fs, "", 0);   // Mount gọi USER_initialize()
  if (fr != FR_OK) {
	  printf("Mount failed: %d\n", fr);
	  Error_Handler();
  }

  if(ADS1115_init(&my_ADS1115_1, ADS1115_ADRR_ADS1115_GND, ADS1115_A0) ==3) printf("using FIR to reduce noise\r\n");
  if(ADS1115_init(&my_ADS1115_1, ADS1115_ADRR_ADS1115_GND, ADS1115_A1) ==3) printf("using FIR to reduce noise\r\n");
  if(ADS1115_init(&my_ADS1115_1, ADS1115_ADRR_ADS1115_GND, ADS1115_A2) ==3) printf("using FIR to reduce noise\r\n");
  if(ADS1115_init(&my_ADS1115_1, ADS1115_ADRR_ADS1115_GND, ADS1115_A3) ==3) printf("using FIR to reduce noise\r\n");
  if(ADS1115_init(&my_ADS1115_1, ADS1115_ADRR_ADS1115_VCC, ADS1115_A0) ==3) printf("using FIR to reduce noise\r\n");
  if(ADS1115_init(&my_ADS1115_1, ADS1115_ADRR_ADS1115_VCC, ADS1115_A1) ==3) printf("using FIR to reduce noise\r\n");
  if(ADS1115_init(&my_ADS1115_1, ADS1115_ADRR_ADS1115_VCC, ADS1115_A2) ==3) printf("using FIR to reduce noise\r\n");
  if(ADS1115_init(&my_ADS1115_1, ADS1115_ADRR_ADS1115_VCC, ADS1115_A3) ==3) printf("using FIR to reduce noise\r\n");
  DS18B20_init();


  // Mở file 1 lần
  fr = f_open(&file, "data.csv", FA_OPEN_ALWAYS | FA_WRITE);
  if(fr != FR_OK){
      printf("Open file failed: %d\n", fr);
  }

  // Đưa con trỏ ghi xuống cuối file
  fr = f_lseek(&file, f_size(&file));
  if(fr != FR_OK){
      printf("Seek failed: %d\n", fr);
      f_close(&file);
  }
  // Ghi file
  const char header[] = "s,m,h,date,month,year,temp,hum,rs/r0mq3,rs/r0mq4,rs/r0mq5,rs/r0mq6,rs/r0mq7,rs/r0mq8,rs/r0mq9,rs/r0mq135,tempW\n";  // 9 ký tự, có newline
  fr = f_write(&file, header, strlen(header), &bw);  // strlen(header) = 9
  if(fr != FR_OK || bw != strlen(header)){
      printf("Write failed\n");
  }
  f_sync(&file);

  SetpointHeat = 40.0;
  PID(&PIDHeat, &InputHeat, &OutputHeat, &SetpointHeat, 0.3805, 0.0015, 3,_PID_P_ON_E, _PID_CD_DIRECT);
  PID_SetMode(&PIDHeat, _PID_MODE_AUTOMATIC);
  PID_SetSampleTime(&PIDHeat, 2000);
  PID_SetOutputLimits(&PIDHeat, 0, 1);

  SetpointHum = 65.0;
  PID(&PIDHum, &InputHum, &OutputHum, &SetpointHum, 0.05,0.0034 , 2.5, _PID_P_ON_E, _PID_CD_REVERSE); //0.3,0.008 , 3
  PID_SetMode(&PIDHum, _PID_MODE_AUTOMATIC);
  PID_SetSampleTime(&PIDHum, 3000);
  PID_SetOutputLimits(&PIDHum, 0, 1);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1); //heat
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2); //cold
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3); //air pump
  int is_first = 1;
  uint32_t roottime = 0;
  uint8_t is_on_pump = 0;
  uint8_t is_on_hum = 0;
  HAL_UART_Receive_IT(&huart2, &rx, 1);
  
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    TASK_data_sensor();
    
    if(DS18B20_start_conversion(&temperature)) continue;

////////////////////////////////////////////////////////////////////////////////////////////////////////////

      char bufHeat[10];
      char bufHum[10];
      InputHeat = (double)temperature;
      PID_Compute(&PIDHeat);
      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, (uint32_t)(OutputHeat * 9999));

      
      InputHum = (double)my_variable_capture_buf.sensor_data.humidity;
      PID_Compute(&PIDHum);
      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, (uint32_t)(OutputHum * 9999));
      

      float_to_str_2int(my_variable_capture_buf.sensor_data.humidity, bufHum);
      float_to_str_2int(temperature, bufHeat);
      printf("PMW Heat %lu, PMW Cold %lu, Temp %s C, Hum %s %%\r\n", (uint32_t)(OutputHeat * 9999), (uint32_t)(OutputHum * 9999), bufHeat, bufHum);

     

      if(rx=='a') {
        is_on_pump? (is_on_pump = 0): (is_on_pump = 1);
        rx = 0;
        fr = f_write(&file, "pump start\n",  strlen("pump start\n"), &bw);
        if(fr != FR_OK || bw !=  strlen("pump start\n")){
            printf("Write failed\n");
        }
        f_sync(&file);
      }
      if(rx=='b') {
        rx = 0;
        fr = f_write(&file, "start\n",  strlen("start\n"), &bw);
        if(fr != FR_OK || bw !=  strlen("start\n")){
            printf("Write failed\n");
        }
        f_sync(&file);
      }

      if(is_on_pump){
          __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, (uint32_t)9999);
      }
      else {
          __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, (uint32_t)0);
      }

      
      

//////////////////////////////////////////////////////////////////////////////////////////////////////
      // if(rx=='b') {
      //   is_on? (is_on = 0): (is_on = 1);
      //   rx = 0;
      // }

      // if(is_on) __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 9999);
      // else {
      //   __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
      //   float_to_str_2int(temperature, buf);
      //   printf("PMW %lu, temp %s\r\n", (uint32_t)(OutputHeat * 9999),buf);
      // }

      // if(is_first){
      //     is_first = 0;
      //     if(RTC_read(&rtcTime) != 0) printf("RTC read error\r\n");
      //     roottime = rtcTime.seconds + rtcTime.minutes*60 + rtcTime.hours*3600;
      //     continue;
      // }

      // if(RTC_read(&rtcTime) != 0) printf("RTC read error\r\n");
      

      // my_func_start_get_data_DHT22(&my_variable_capture_buf);
  
      // float_to_str_2int(my_variable_capture_buf.sensor_data.humidity, buf);
      // uint32_t time = rtcTime.seconds + rtcTime.minutes*60 + rtcTime.hours*3600 - roottime;
      // printf("%u,%s\n",time,buf);
////////////////////////////////////////////////////////////////////////////////////
    DS18B20_delay_s(1);
    /* USER CODE END WHILE */
    
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_8;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 639;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 9999;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_FALLING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim2, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 63;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);
  /* DMA1_Channel3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);
  /* DMA1_Channel5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI1_CS_Pin */
  GPIO_InitStruct.Pin = SPI1_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SPI1_CS_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void wait_to_start_mq(uint32_t wait_time_s){
    
    do{
      HAL_Delay(1000);
      printf("wait to start mq time remaining : %u s\n", wait_time_s);
    }while (--wait_time_s);
}

  void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        // xử lý dữ liệu rx ở đây

        // nhận tiếp byte tiếp theo
        HAL_UART_Receive_IT(&huart2, &rx, 1);
    }
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
     if (htim->Instance == TIM2) {
          my_variable_capture_buf.is_data_ready = 1;
          HAL_TIM_IC_Stop_DMA(&htim2, TIM_CHANNEL_1);
     }
}
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi){
	if(hspi->Instance == SPI1) {
		// Dữ liệu TX đã xong
		mySD_handle1.is_spi_tx_ready = 1;
	}
}
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1) {
        // Bật cờ RX vì quá trình TransmitReceive đã hoàn tất cả việc gửi và nhận
        mySD_handle1.is_spi_rx_ready = 1;
    }
}

void TASK_data_sensor(void){
    // Đọc dữ liệu cảm biến khác và xử lý nếu cần
    //DHT22
    ////////////////////////////////////////////////////////////////////////////////////////////
    my_func_start_get_data_DHT22(&my_variable_capture_buf);

//		char temp_str[6];
//		char hum_str[6];
//		float_to_str_xx_xx(my_variable_capture_buf.sensor_data.temperature, temp_str);
//		float_to_str_xx_xx(my_variable_capture_buf.sensor_data.humidity, hum_str);
//		printf("data:%s,%s\n",temp_str,hum_str);

    //MQ data
    ////////////////////////////////////////////////////////////////////////////////////////////
    if(ADS1115_getADC(&my_ADS1115_1, ADS1115_ADRR_ADS1115_GND, ADS1115_A0)!=0) printf("Error reading ADC GND A0\r\n");
    HAL_Delay(100);
    if(ADS1115_getADC(&my_ADS1115_1, ADS1115_ADRR_ADS1115_GND, ADS1115_A1)!=0) printf("Error reading ADC GND A1\r\n");
    HAL_Delay(100);
    if(ADS1115_getADC(&my_ADS1115_1, ADS1115_ADRR_ADS1115_GND, ADS1115_A2)!=0) printf("Error reading ADC GND A2\r\n");
    HAL_Delay(100);
    if(ADS1115_getADC(&my_ADS1115_1, ADS1115_ADRR_ADS1115_GND, ADS1115_A3)!=0) printf("Error reading ADC GND A3\r\n");
    HAL_Delay(100);
    if(ADS1115_getADC(&my_ADS1115_1, ADS1115_ADRR_ADS1115_VCC, ADS1115_A0)!=0) printf("Error reading ADC VCC A0\r\n");
    HAL_Delay(100);
    if(ADS1115_getADC(&my_ADS1115_1, ADS1115_ADRR_ADS1115_VCC, ADS1115_A1)!=0) printf("Error reading ADC VCC A1\r\n");
    HAL_Delay(100);
    if(ADS1115_getADC(&my_ADS1115_1, ADS1115_ADRR_ADS1115_VCC, ADS1115_A2)!=0) printf("Error reading ADC VCC A2\r\n");
    HAL_Delay(100);
    if(ADS1115_getADC(&my_ADS1115_1, ADS1115_ADRR_ADS1115_VCC, ADS1115_A3)!=0) printf("Error reading ADC VCC A3\r\n");
    HAL_Delay(100);

		//RTC data
    ////////////////////////////////////////////////////////////////////////////////////////////
    if(RTC_read(&rtcTime) != 0) printf("RTC read error\r\n");

    // chuẩn bị ghi dữ liệu
    char buff[254];
    char s[10];
    char *p = buff;
    

    /* Time: SS,MM,HH,DD,MM,YYYY */
    p = append_2d(p, rtcTime.seconds); *p++ = ',';
    p = append_2d(p, rtcTime.minutes); *p++ = ',';
    p = append_2d(p, rtcTime.hours);   *p++ = ',';
    p = append_2d(p, rtcTime.date);    *p++ = ',';
    p = append_2d(p, rtcTime.month);   *p++ = ',';

    *p++ = '2'; *p++ = '0';
    p = append_2d(p, rtcTime.year);
    *p++ = ',';

    /* Float data */
    float_to_str_2int(my_variable_capture_buf.sensor_data.temperature, s);
    p = append_str(p, s); *p++ = ',';

    float_to_str_2int(my_variable_capture_buf.sensor_data.humidity, s);
    p = append_str(p, s); *p++ = ',';

    float_to_str_2int(my_ADS1115_1.GND.A0.Rs_R0, s);
    p = append_str(p, s); *p++ = ',';

    float_to_str_2int(my_ADS1115_1.GND.A1.Rs_R0, s);
    p = append_str(p, s); *p++ = ',';

    float_to_str_2int(my_ADS1115_1.GND.A2.Rs_R0, s);
    p = append_str(p, s); *p++ = ',';

    float_to_str_2int(my_ADS1115_1.GND.A3.Rs_R0, s);
    p = append_str(p, s); *p++ = ',';

    float_to_str_2int(my_ADS1115_1.VCC.A0.Rs_R0, s);
    p = append_str(p, s); *p++ = ',';

    float_to_str_2int(my_ADS1115_1.VCC.A1.Rs_R0, s);
    p = append_str(p, s); *p++ = ',';

    float_to_str_2int(my_ADS1115_1.VCC.A2.Rs_R0, s);
    p = append_str(p, s); *p++ = ',';

    float_to_str_2int(my_ADS1115_1.VCC.A3.Rs_R0, s);
    p = append_str(p, s); *p++ = ',';

    float_to_str_2int(temperature, s);
    p = append_str(p, s);

    *p++ = '\n';
    *p = '\0';

    //ghi dữ liệu

    fr = f_write(&file, buff,  strlen(buff), &bw);
        if(fr != FR_OK || bw !=  strlen(buff)){
            printf("Write failed\n");
            return;
        }
        f_sync(&file);
        printf("%s",buff);


    /* Theo dõi stack: Bật configUSE_TRACE_FACILITY=1 và INCLUDE_uxTaskGetStackHighWaterMark=1 trong FreeRTOSConfig.h */
   
   // UBaseType_t free_words = uxTaskGetStackHighWaterMark(NULL);
   // printf("Stack free in TASK_data_sensor (words): %lu\n", free_words);

  }



/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
