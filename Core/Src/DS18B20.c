#include "DS18B20.h"

void DS18B20_delay_us(uint16_t us)
{
    uint32_t start = __HAL_TIM_GET_COUNTER(&DS18B20_TIM_BASE);
    while (__HAL_TIM_GET_COUNTER(&DS18B20_TIM_BASE)- start < us);
}

void DS18B20_delay_ms(uint16_t ms)
{
    while (ms--) DS18B20_delay_us(1000);
}

void DS18B20_delay_s(uint16_t s)
{
    while (s--) DS18B20_delay_ms(1000);
}


void DS18B20_check_time(uint16_t start_time ,uint16_t end_time, uint16_t *result_time)
{
  if(end_time >= start_time) *result_time = end_time - start_time;
  else *result_time = (0xFFFF - start_time) + end_time;
}


int DS18B20_reset(void){
  uint16_t time_out = DS18B20_TIME_OUT_US;
  uint16_t start_time;
  uint16_t time;
  uint16_t end_time;

  DS18B20_HOLD_LOW();
  DS18B20_delay_us(600); // Giữ mức thấp trong ít nhất 480 microgiây
  DS18B20_RELEASE();
  DS18B20_delay_us(60);

  start_time = __HAL_TIM_GET_COUNTER(&DS18B20_TIM_BASE);
  do{
    if(HAL_GPIO_ReadPin(DS18B20_GPIO_PORT, DS18B20_PIN)) {
      end_time = __HAL_TIM_GET_COUNTER(&DS18B20_TIM_BASE);
      break;
    }
  }while(--time_out);
  if(time_out == 0) return 1; // time out error
  
  DS18B20_check_time(start_time, end_time, &time);

  if(!(time > DS18B20_SIGNAL_RESPONE_PRESENCE_MIN_US && time < DS18B20_SIGNAL_RESPONE_PRESENCE_MAX_US)) return 2;// Không phải xung Presence

  return 0; // ok
}

void DS18B20_transmit_data(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        // Gửi bit 1 (LSB trước)
        if (data & 0x01) {
            DS18B20_HOLD_LOW();
            DS18B20_delay_us(2); // Kéo thấp khoảng 2us (chuẩn >1us)
            DS18B20_RELEASE();  // Thả chân ra để trở treo kéo lên
            DS18B20_delay_us(DS18B20_SIGNAL_CYCLE_US);  // Đợi hết khe thời gian (tổng khoảng 60us)
        } 
        // Gửi bit 0
        else {
            DS18B20_HOLD_LOW();
            DS18B20_delay_us(DS18B20_SIGNAL_CYCLE_US);  // Giữ thấp trong phần lớn khe thời gian
            DS18B20_RELEASE();   // Thả chân ra
            DS18B20_delay_us(2);  // Thời gian hồi phục (recovery time)
        }
        data >>= 1; // Chuyển sang bit tiếp theo
    }
}

void DS18B20_receive_data(uint8_t *data) {
    uint8_t result = 0; // Khởi tạo bằng 0
    
    for (uint8_t i = 0; i < 8; i++) {
        
        DS18B20_HOLD_LOW();
        DS18B20_delay_us(2);    
        DS18B20_RELEASE();
        DS18B20_delay_us(5); 
  
        if (HAL_GPIO_ReadPin(DS18B20_GPIO_PORT, DS18B20_PIN)) {
            result |= (1 << i); // Nếu đọc được 1, set bit thứ i lên 1
        }
        DS18B20_delay_us(DS18B20_SIGNAL_CYCLE_US); 
    }
    *data = result; 
}

int DS18B20_init(void){
  HAL_TIM_Base_Start(&DS18B20_TIM_BASE);
  if(DS18B20_reset())return 1;
  DS18B20_transmit_data(0xCC); // Skip ROM
  DS18B20_transmit_data(0x4E); // Write Scratchpad
  DS18B20_transmit_data(0x00);  
  DS18B20_transmit_data(0x00);
  DS18B20_transmit_data(DS18B20_RESOLUTION); // 12 bit resolution
  return 0; // ok
}

int DS18B20_start_conversion(float *temperature) {
    uint8_t lsb;
    uint8_t msb;
    uint16_t temp_raw; 
    static uint8_t temperature_out =10;
    static float last_conversion_temperature = 0;

    if (DS18B20_reset()) return 1; 
    DS18B20_transmit_data(0xCC); 
    DS18B20_transmit_data(0x44); 
    DS18B20_delay_s(1);
    
    if (DS18B20_reset()) return 2; 
    DS18B20_transmit_data(0xCC);
    DS18B20_transmit_data(0xBE); 

    DS18B20_receive_data(&lsb); 
    DS18B20_receive_data(&msb); 
    
    temp_raw = (msb << 8) | lsb;
    *temperature = (int16_t)temp_raw / 16.0;
    if(temperature_out--) if((*temperature>(last_conversion_temperature+5))&&(*temperature<(last_conversion_temperature-5))) return 1;

    temperature_out=10;
    last_conversion_temperature = *temperature;

    return 0; // Thành công
}