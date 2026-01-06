/*
 * DHT22.c
 *
 *  Created on: Nov 20, 2025
 *      Author: k
 */


#include "DHT22.h"

/* External Variables --------------------------------------------------------*/
/* Đảm bảo bạn đã khai báo các biến này trong main.c */




/* Private Variables ---------------------------------------------------------*/
/* Con trỏ dùng để trỏ tới buffer đang xử lý, giúp Callback biết ghi vào đâu */
static my_struct_data_captrure *p_dht22_capture_handle = NULL;

/* Function Implementation ---------------------------------------------------*/

int my_func_start_get_data_DHT22(my_struct_data_captrure *capture_buf)
{
    HAL_TIM_Base_Start(&MY_TIM_BASE);
    my_func_start_GPIO_Output();
    HAL_GPIO_WritePin(GPIO_SOURCE, MY_GPIO_INPUT_CAPTURE_MODE , 0); // Kéo xuống 0
    my_func_delay_ms(1); // DHT22 chỉ cần 1ms thay vì 18ms như DHT11
    HAL_GPIO_DeInit(GPIO_SOURCE, MY_GPIO_INPUT_CAPTURE_MODE);  // Thả ra (nên sẽ lên 1) [cite: 15]

    my_func_start_tim2(capture_buf);

    // Chờ dữ liệu với timeout
    uint16_t timeout = 1000; // 1000 vòng lặp timeout
    while(!capture_buf->is_data_ready && timeout--)
    {
         my_func_delay_us(10); // Tối ưu: chỉ delay ngắn thay vì HAL_Delay
    }

    // Reset cờ sau khi nhận xong
    capture_buf->is_data_ready = 0;
    
    // Decode và lưu kết quả vào biến toàn cục hoặc truyền vào

    my_func_decode_DHT22_signal(capture_buf);
    return 0;
}
// val >= 0
// buf: phải có ít nhất 6 bytes
void float_to_str_xx_xx(float val, char *buf)
{

    if(val < 0) val = -val; // đảm bảo luôn dương

    // làm tròn xuống 2 chữ số thập phân
    int total = (int)(val * 100 + 0.5f);  // ví dụ 5.37 -> 537
    int whole = total / 100;              // phần nguyên
    int frac  = total % 100;              // phần thập phân

    // chỉ lấy 2 chữ số phần nguyên, cắt nếu >99
    if(whole > 99) whole = 99;

    buf[0] = (whole / 10) + '0';
    buf[1] = (whole % 10) + '0';
    buf[2] = '.';
    buf[3] = (frac / 10) + '0';
    buf[4] = (frac % 10) + '0';
    buf[5] = '\0';
}



void my_func_start_GPIO_Output(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    HAL_GPIO_WritePin(GPIO_SOURCE, MY_GPIO_INPUT_CAPTURE_MODE, 1);

    GPIO_InitStruct.Pin = MY_GPIO_INPUT_CAPTURE_MODE;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; // [cite: 17]
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIO_SOURCE, &GPIO_InitStruct);
}

void my_func_start_tim2(my_struct_data_captrure *capture_buf)
{
    // Gán con trỏ toàn cục để dùng trong ngắt
    p_dht22_capture_handle = capture_buf;

    // Bắt đầu DMA
    HAL_TIM_IC_Start_DMA(&MY_TIM_INPUT_CAPTURE_MODE, MY_TIM_INPUT_CAPTURE_MODE_CHANNEL, (uint32_t *)capture_buf->capture_buf, MY_DMA_REQUEST_DHT22_SIZE);
}

void my_func_deinti_all(void){
    // Code gốc chưa có nội dung
}

void my_func_delay_us(uint16_t us)
{
    uint32_t start = __HAL_TIM_GET_COUNTER(&MY_TIM_BASE);
    while (__HAL_TIM_GET_COUNTER(&MY_TIM_BASE)- start < us);
}

void my_func_delay_ms(uint16_t ms)
{
    while (ms--) my_func_delay_us(1000); // [cite: 20]
}

void my_func_delay_s(uint16_t s)
{
    while(s--) my_func_delay_ms(1000);
}

void my_func_decode_DHT22_signal(my_struct_data_captrure *data)
{
    int is_data_ready = -1;
    uint8_t count = 0;
    
    // Mảng lưu 5 byte dữ liệu DHT22
    uint8_t bytes[5] = {0};
    uint8_t bit_idx = 0;

    // Các ngưỡng tần số - dùng uint32_t tiết kiệm bộ nhớ
    const uint32_t signal_start_frequency_max = SIGNAL_START_FREQUENCY + (SIGNAL_START_FREQUENCY * FREQ_TOLERANCE_PERCENT / 100);
    const uint32_t signal_start_frequency_min = SIGNAL_START_FREQUENCY - (SIGNAL_START_FREQUENCY * FREQ_TOLERANCE_PERCENT / 100);
    const uint32_t signal_0_frequency_max = SIGNAL_0_FREQUENCY + (SIGNAL_0_FREQUENCY * FREQ_TOLERANCE_PERCENT / 100);
    const uint32_t signal_0_frequency_min = SIGNAL_0_FREQUENCY - (SIGNAL_0_FREQUENCY * FREQ_TOLERANCE_PERCENT / 100);
    const uint32_t signal_1_frequency_max = SIGNAL_1_FREQUENCY + (SIGNAL_1_FREQUENCY * FREQ_TOLERANCE_PERCENT / 100);
    const uint32_t signal_1_frequency_min = SIGNAL_1_FREQUENCY - (SIGNAL_1_FREQUENCY * FREQ_TOLERANCE_PERCENT / 100);

    uint8_t max_idx = (MY_DMA_REQUEST_DHT22_SIZE < MY_DMA_BUFFER_SIZE - 1) ? MY_DMA_REQUEST_DHT22_SIZE : MY_DMA_BUFFER_SIZE - 1;
	#if DEBUG_DHT_22
    printf("Frequencies: ");
	#endif
    for (uint8_t i = 0; i < max_idx; i++)
    {
        my_struct_pulse_frequency pulse_info = {0};
        my_func_caculator_frequency(data->capture_buf[i], data->capture_buf[i + 1], &pulse_info);
        uint32_t freq = pulse_info.frequency;

        
	#if DEBUG_DHT_22
            // In tần số (không lưu)
            char buf[12];  // uint32 tối đa 10 chữ số + null
            my_func_uint32_to_str(freq, buf);
        	printf("%s ", buf);
		#endif
        if (is_data_ready < 0)
        {
            if (signal_start_frequency_min < freq && freq < signal_start_frequency_max)
            {
                is_data_ready = i;
				#if DEBUG_DHT_22
                printf("[START] ");
				#endif
            }
        }
        else if(count < 40)
        {
            uint8_t bit_value = 0;
            if (signal_1_frequency_min < freq && freq < signal_1_frequency_max)
            {
                bit_value = 1;
				#if DEBUG_DHT_22
                printf("1");
				#endif

            }
            else if (signal_0_frequency_min < freq && freq < signal_0_frequency_max)
            {
				#if DEBUG_DHT_22
                bit_value = 0;
                printf("0");
				#endif
            }
            else
            {
                continue; // Bỏ qua tần số không hợp lệ
            }
            
            // Lưu bit vào byte tương ứng
            uint8_t byte_idx = bit_idx / 8;
            uint8_t bit_pos = 7 - (bit_idx % 8);
            bytes[byte_idx] |= (bit_value << bit_pos);
            
            bit_idx++;
            count++;
        }
    }
	#if DEBUG_DHT_22
    printf("\n");
	#endif
    // Kiểm tra checksum
    uint8_t calc_chk = (bytes[0] + bytes[1] + bytes[2] + bytes[3]) & 0xFF;
    
    if(calc_chk == bytes[4] && count == 40)
    {
        // Ghép 2 byte độ ẩm
        uint16_t hum_raw = (bytes[0] << 8) | bytes[1];
        data->sensor_data.humidity = hum_raw / 10.0f;
        
        // Ghép 2 byte nhiệt độ (có thể âm)
        uint16_t temp_raw = (bytes[2] << 8) | bytes[3];
        if (temp_raw & 0x8000) // Bit dấu
        {
        	data->sensor_data.temperature = -((temp_raw & 0x7FFF) / 10.0f);
        }
        else
        {
        	data->sensor_data.temperature = temp_raw / 10.0f;
        }
    }
    else
    {
		#if DEBUG_DHT_22
        printf("Checksum error! (calc=%d, recv=%d, bits=%d)\n", calc_chk, bytes[4], count);
		#endif
        data->sensor_data.temperature = 0.0f;
        data->sensor_data.humidity = 0.0f;
    }
}

// Helper function local to this file
static uint64_t reverse_40bit(uint64_t value)
{
    uint64_t reversed = 0;
    for (int i = 0; i < 40; i++) // [cite: 45]
    {
        if (value & (1ULL << i))
            reversed |= 1ULL << (39 - i);
    }
    return reversed;
}

void my_func_print_hex_uint64(uint64_t value, char *out)
{
    uint64_t data = reverse_40bit(value);
    for (int i = 0; i < 5; i++) // [cite: 47]
    {
        uint8_t byte = (data >> ((4 - i) * 8)) & 0xFF;
        out[i*2]   = (byte >> 4) < 10 ? '0' + (byte >> 4) : 'A' + ((byte >> 4) - 10); // [cite: 48]
        out[i*2+1] = (byte & 0x0F) < 10 ? '0' + (byte & 0x0F) : 'A' + ((byte & 0x0F) - 10);
    }
    out[10] = '\0';
}



void my_func_caculator_frequency(uint16_t counter_truoc, uint16_t counter_sau, my_struct_pulse_frequency *pulse_freq)
{
    if(counter_truoc == counter_sau) return;

    // Tính delta với xử lý overflow
    uint32_t delta = (counter_sau > counter_truoc)
                         ? (counter_sau - counter_truoc)
                         : (0x10000 - counter_truoc + counter_sau);

    if(delta == 0) return; // Tránh chia cho 0

    // Tối ưu: tính trực tiếp tần số (prescaler = 0), uint32_t đủ cho 64MHz
    pulse_freq->frequency = (uint32_t)(MY_TIM_INPUT_CAPTURE_MODE_CLOCK_FREQ / delta);
}


void my_func_uint32_to_str(uint32_t value, char *str)
{
    if (value == 0)
    {
        str[0] = '0';
        str[1] = '\0';
        return;
    }
    
    // Tối ưu: buffer cục bộ cho uint32 (11 ký tự đủ)
    char buffer[11];
    uint8_t i = 0;
    
    while (value > 0)
    {
        buffer[i++] = (value % 10) + '0';
        value /= 10;
    }
    
    // Đảo ngược vào str
    uint8_t j = 0;
    while (i > 0)
    {
        str[j++] = buffer[--i];
    }
    str[j] = '\0';
}












