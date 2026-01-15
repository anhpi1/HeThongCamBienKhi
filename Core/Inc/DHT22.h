/*
 * DHT22.h
 *
 *  Created on: Nov 20, 2025
 *      Author: k
 */

// ========================== Hướng dẫn sử dụng (rõ ràng, ngắn gọn) ==========================
/*
1) Cấu hình CubeMX (tóm tắt)
    - System clock: 64 MHz.
    - Bật chế độ debug để dễ gỡ lỗi.
    - GPIO PC13 (hoặc bất kỳ LED báo) cấu hình OUTPUT để kiểm tra MCU còn chạy.
    - TIM2: Chế độ Input Capture, Channel 1. Nếu thay đổi prescaler thì cập nhật
      MY_TIM_INPUT_CAPTURE_MODE_CLOCK_FREQ tương ứng. Để chế độ nguồn nội, bật ngắt, chọn chế độ nhận diện cạnh xuống
      - Bật DMA cho TIM2 Channel 1 (DMA request).
    - TIM3: Base timer dùng làm bộ đếm chung (delay). Đặt prescaler = 64-1 để có
      xung 1 MHz (tick = 1 us) khi timer được cấp 64 MHz.
    - Bật USART1 (hoặc UART mong muốn) để dùng printf cho debug.

2) Hàm printf qua UART
    - Thêm hàm _write như sau để printf hoạt động (đặt vào User Code):
int _write(int file, char *ptr, int len)
{
     HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
     return len;
}

3) Biến lưu dữ liệu capture (global)
    - Tạo biến toàn cục để DMA/IC ghi dữ liệu:
my_struct_data_captrure my_variable_capture_buf = {0};

4) Callback khi DMA/IC hoàn thành
    - Trong HAL_TIM_IC_CaptureCallback đánh dấu dữ liệu sẵn sàng và dừng DMA:
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
     if (htim->Instance == TIM2) {
          my_variable_capture_buf.is_data_ready = 1;
          HAL_TIM_IC_Stop_DMA(&htim2, TIM_CHANNEL_1);
     }
}

5) Sử dụng trong vòng lặp chính (ví dụ)

    my_func_start_get_data_DHT11(&my_variable_capture_buf);
	my_func_delay_s(1);


*/
// ==========================================================================================


#ifndef INC_DHT22_H_
#define INC_DHT22_H_

#include "stdint.h"
#include "stdio.h"
#include <stdbool.h>
#include "stm32f1xx_hal.h"

typedef uint8_t bool_;

// ========================== SETTINGS ==========================

#define DEBUG_DHT_22 0
#define MY_TIM_BASE                     		htim3 					// Timer dùng làm timer chính (bộ đếm thời gian chung)
#define MY_TIM_INPUT_CAPTURE_MODE      			htim2 					// Timer dùng cho Input Capture (đo xung / tần số)
#define GPIO_SOURCE                     		GPIOA 					// GPIO port dùng để nhận tín hiệu
#define MY_GPIO_INPUT_CAPTURE_MODE      		GPIO_PIN_0 				// GPIO pin đầu vào cho Input Capture
#define MY_TIM_INPUT_CAPTURE_MODE_CLOCK_FREQ   	64000000UL 				// Tần số clock cấp cho Timer đo Input Capture (Hz)
#define MY_TIM_INPUT_CAPTURE_MODE_CHANNEL      	TIM_CHANNEL_1 			// Kênh Input Capture sử dụng (CH1 / CH2 / CH3 / CH4)
#define MY_DMA_BUFFER_SIZE              		43 						// Kích thước buffer DMA - tối ưu cho DHT22 (42 cạnh + 1)
#define MY_DMA_REQUEST_DHT22_SIZE       		42 						// Số lượng cạnh cần đọc từ DMA (DHT22 dùng 42 cạnh)
#define SIGNAL_START_FREQUENCY          		7097 					// Tần số phát hiện tín hiệu START (Hz)
#define SIGNAL_0_FREQUENCY              		14096 					// Tần số biểu diễn bit 0 (Hz)
#define SIGNAL_1_FREQUENCY              		8880 					// Tần số biểu diễn bit 1 (Hz)
#define FREQ_TOLERANCE_PERCENT          		5   					// Dung sai tần số (±%)
// ================================================================

extern TIM_HandleTypeDef MY_TIM_INPUT_CAPTURE_MODE;
extern TIM_HandleTypeDef MY_TIM_BASE;

typedef struct{
uint32_t frequency;  // Tần số (Hz) - 32-bit đủ cho 64MHz
} my_struct_pulse_frequency;

typedef struct {
    float temperature;     // Nhiệt độ (°C), có thể âm
    float humidity;        // Độ ẩm (%)
} my_struct_DHT22_data;


typedef struct{
uint16_t capture_buf[MY_DMA_BUFFER_SIZE];
uint8_t is_data_ready;
my_struct_DHT22_data sensor_data;
}my_struct_data_captrure;


void my_func_start_get_data_DHT22(my_struct_data_captrure *capture_buf);
void my_func_decode_DHT22_signal(my_struct_data_captrure *data);
void my_func_start_GPIO_Output(void);


//hàm phụ chuyển số nguyên sang string
void my_func_uint32_to_str(uint32_t value, char *str);
void my_float_to_str_1dp(float value, char *buf);

//các hàm tính toán  tần số
void my_func_caculator_frequency(uint16_t counter_truoc, uint16_t counter_sau, my_struct_pulse_frequency *my_struct_pulse_frequency);//

void my_func_print_hex_uint64(uint64_t value, char *out);

void my_func_delay_s(uint16_t s);
void my_func_delay_ms(uint16_t ms);
void my_func_delay_us(uint16_t us);
void my_func_start_tim2(my_struct_data_captrure *capture_buf);



#endif /* INC_DHT22_H_ */
