#include "ADS1115.h"
#include <stdint.h>

void ADS1115_wait(uint8_t DR){
	switch (DR)
	    {
	        case 0b000: HAL_Delay(125); break;// 8 SPS
	        case 0b001: HAL_Delay(63); break; // 16 SPS
	        case 0b010: HAL_Delay(32); break; // 32 SPS
	        case 0b011: HAL_Delay(16); break; // 64 SPS
	        case 0b100: HAL_Delay(8); break;  // 128 SPS
	        case 0b101: HAL_Delay(4); break;  // 250 SPS
	        case 0b110: HAL_Delay(3); break;  // 475 SPS
	        case 0b111: HAL_Delay(2); break;  // 860 SPS
	        default:    HAL_Delay(10);
	    }
}
#if ADS1115_ENABLE_ADC
int ADS1115_adc_read_once(uint16_t *value)
{
    if(HAL_ADC_Start(ADS1115_hadc) != HAL_OK) return 1;                      // Bắt đầu ADC
    if(HAL_ADC_PollForConversion(ADS1115_hadc, 10) != HAL_OK) return 2;      // Chờ ADC xong
    *value = HAL_ADC_GetValue(ADS1115_hadc);           // Lấy giá trị
    if(HAL_ADC_Stop(ADS1115_hadc) != HAL_OK) return 3;                       // Dừng ADC
    return 0;
}
#endif

int ADS1115_read_r0(ADS1115_handle *my_ADS1115, uint8_t adrrDevice, uint8_t channel){
    ADS1115_device *device;
    ADS1115_channel *ch;
    if(ADS1115_decode_device_address(adrrDevice, &device)!=0) return 3;
    if(ADS1115_decode_channel_address(device, channel, &ch)!=0) return 4;
    
	  my_ADS1115->data[0] = 0b10000000 | (channel << 4);
	  my_ADS1115->data[1] = 0b00000011 | (0 << 5);

    if(HAL_I2C_Mem_Write(ADS1115_hi2c, adrrDevice << 1, ADS1115_ARR_REG_CONFIG, I2C_MEMADD_SIZE_8BIT, my_ADS1115->data, 2, 100)!= HAL_OK) return 1;


	  my_ADS1115->data[0] = ADS1115_ARR_REG_CONVERSION;
    for(uint8_t i=0;i<10;i++){
	    ADS1115_wait(000);
      if(HAL_I2C_Mem_Read(ADS1115_hi2c, adrrDevice << 1, ADS1115_ARR_REG_CONVERSION, I2C_MEMADD_SIZE_8BIT, my_ADS1115->data, 2, 100)!= HAL_OK) return 2;
    }

    
	  int16_t value = (my_ADS1115->data[0] << 8) | my_ADS1115->data[1];
    float vout = (float)value * 0.0001875f;
    ch->R0 =  ADS1115_RL*(my_ADS1115->vontage_ref - vout) / vout;
    #if ADS1115_DEBUG
    printf("R0 channel %d = %d ohm,%d,%d\r\n", channel & 0b011, (int)ch->R0,(int)(my_ADS1115->vontage_ref*100),(int)(vout*100));
    #endif
    return 0;
}

int ADS1115_decode_device_address(uint8_t adrrDevice, ADS1115_device **device){
  if(adrrDevice == ADS1115_ADRR_ADS1115_GND){
    *device = &my_ADS1115_1.GND;
    return 0;
  }
  else if(adrrDevice == ADS1115_ADRR_ADS1115_VCC){
    *device = &my_ADS1115_1.VCC;
    return 0;
  }
  else return 1;
}
int  ADS1115_decode_channel_address(ADS1115_device *device, uint8_t channel, ADS1115_channel **ch){
  switch(channel & 0b011){
    case 0b00:
      *ch = &device->A0;
      return 0;
    case 0b01:
      *ch = &device->A1;
      return 0;
    case 0b10:
      *ch = &device->A2;
      return 0;
    case 0b11:
      *ch = &device->A3;
      return 0;
    default:
      return 1;
  }
}

int ADS1115_getADC(ADS1115_handle *my_ADS1115,uint8_t adrrDevice, uint8_t channel){
    
    ADS1115_device *device;
    ADS1115_channel *ch;
    if(ADS1115_decode_device_address(adrrDevice, &device)!=0) return 3;
    if(ADS1115_decode_channel_address(device, channel, &ch)!=0) return 4;
    if(ch->R0 <= 0.0f) return 5; // Kiểm tra R0 hợp lệ
	  my_ADS1115->data[0] = 0b10000000 | (channel << 4);
	  my_ADS1115->data[1] = 0b00000011 | (ch->DR << 5);

    if(HAL_I2C_Mem_Write(ADS1115_hi2c, adrrDevice << 1, ADS1115_ARR_REG_CONFIG, I2C_MEMADD_SIZE_8BIT, my_ADS1115->data, 2, 100)!= HAL_OK) return 1;


	  my_ADS1115->data[0] = ADS1115_ARR_REG_CONVERSION;
    for(uint8_t i=0;i<10;i++)
	    ADS1115_wait(ch->DR);
    for(uint8_t i=0;i<10;i++)
      if(HAL_I2C_Mem_Read(ADS1115_hi2c, adrrDevice << 1, ADS1115_ARR_REG_CONVERSION, I2C_MEMADD_SIZE_8BIT, my_ADS1115->data, 2, 100)!= HAL_OK) return 2;


    int16_t value = (my_ADS1115->data[0] << 8) | my_ADS1115->data[1];
    float vout = (float)value * 0.0001875f;
    float Rs =  ADS1115_RL*(my_ADS1115->vontage_ref - vout) / vout;
    ch->Rs_R0 = Rs / ch->R0;
    #if ADS1115_DEBUG
    printf("Channel %d = %d,%d\r\n", channel & 0b011, (int)Rs,(int)ch->R0);
    #endif


	#if ADS1115_DEBUG
	  	  printf("Data reg %s channel %d = %02X %02X\r\n",(adrrDevice==ADS1115_ADRR_ADS1115_GND)?"GND":"VCC",channel & 0b011, my_ADS1115->data[0], my_ADS1115->data[1]);
        printf("Raw value: %d \n", value);
	  	  printf("value: %d.%02d \n", (int)ch->Rs_R0, (int)((ch->Rs_R0 - (int)ch->Rs_R0) * 100));
	#endif
	  return 0;
}
int ADS1115_caculator_normal_probability_distribution(float do_tin_cay, float *resurt) {
    if (do_tin_cay >= 1.0f || do_tin_cay <= 0.0f) return 1;

    float alpha = 1.0f - do_tin_cay;
    float p = alpha / 2.0f; 

    float t = sqrtf(-2.0f * logf(p));
    
    const float c0 = 2.515517f, c1 = 0.802853f, c2 = 0.010328f;
    const float d1 = 1.432788f, d2 = 0.189269f, d3 = 0.001308f;

    float tu_so = c0 + c1 * t + c2 * t * t;
    float mau_so = 1.0f + d1 * t + d2 * t * t + d3 * t * t * t;
    *resurt = t - (tu_so / mau_so);
    return 0;
}

// 2. Sửa lỗi vòng lặp và logic tính trung bình
int ADS1115_get_mean_and_varadiance(float *mean, float *variance, float *arr, uint16_t length){
    if (length < 2) return 1; // Không thể tính phương sai nếu mẫu < 2

    float u = 0.0f;
    float s = 0.0f;
    
    // FIX: Dùng uint16_t cho i để tránh tràn nếu length > 255
    // FIX: i < length (không phải length - 1)
    for(uint16_t i = 0; i < length; i++){
        u += arr[i];
    }
    u = u / length;
  
    for(uint16_t i = 0; i < length; i++){
        s += (arr[i] - u)*(arr[i] - u);
    }

    *mean = u;
    *variance = s / (length - 1); // Phương sai mẫu (chia cho n-1)
    return 0;
}

// 3. Sửa kiểu tham số int -> float cho confidence_level và error_margin
int ADS1115_get_num_sample_required(uint16_t *number_of_samples, float *arr, uint16_t length, float confidence_level, float error_margin){
    float u, s, z; // v ở đây là PHƯƠNG SAI (Variance = s^2)

    if (ADS1115_get_mean_and_varadiance(&u, &s, arr, length) != 0) return 1;
    
    if (ADS1115_caculator_normal_probability_distribution(confidence_level, &z) != 0) return 1;

    #if ADS1115_DEBUG
      printf("Debug: u=%d.%d, s=%d.%d, z=%d.%d\n", (int)u, (int)((u - (int)u) * 100), (int)s, (int)((s - (int)s) * 100), (int)z, (int)((z - (int)z) * 100));
    #endif
    // Công thức: n = (z^2 * s^2) / E^2
    // Vì v = s^2 (phương sai), nên ta dùng v trực tiếp, KHÔNG bình phương v nữa.
    float n_calc = (z * z * s) / (error_margin * error_margin);
    
    // Làm tròn lên (ceil) để đảm bảo đủ mẫu
    *number_of_samples = (int)ceilf(n_calc);
    
    return 0;
}


int ADS1115_init(ADS1115_handle *my_ADS1115, uint8_t adrrDevice, uint8_t channel){
  uint16_t adc_value;
  ADS1115_device *device;
  ADS1115_channel *ch;
  if(ADS1115_decode_device_address(adrrDevice, &device)!=0) return 1;
  if(ADS1115_decode_channel_address(device, channel, &ch)!=0) return 2;
  #if ADS1115_ENABLE_ADC
    if(ADS1115_adc_read_once(&adc_value) != 0) return 3;
  #endif
  #if !ADS1115_ENABLE_ADC
    adc_value = 3970; // Giả sử đọc được 3.0V từ ADC nếu không dùng ADC onboard
  #endif
  my_ADS1115->vontage_ref = ((adc_value*3.3f / 4095.0f)*5/3.3f); // in mV

  ADS1115_read_r0(my_ADS1115, adrrDevice, channel);

 
  ch->DR = 0b111;
  float arr[20];
  for(uint8_t i=0; i<20; i++ ){
      ADS1115_wait(ch->DR);
      if(ADS1115_getADC(my_ADS1115, adrrDevice, channel))return 4;
      arr[i] = ch->Rs_R0;
  }
  uint16_t num_samples;
  if(ADS1115_get_num_sample_required(&num_samples, arr, 20, 0.95f, 0.001f)!=0) return 5;
  printf("Number of samples required calc: %u\n", num_samples);
  ch->DR = (num_samples <= 1) ? 0b111 :
            (num_samples <= 2) ? 0b110 :
            (num_samples <= 4) ? 0b101 :
            (num_samples <= 7) ? 0b100 :
            (num_samples <= 14)  ? 0b011 :
            (num_samples <= 27)  ? 0b010 :
            (num_samples <= 54)  ? 0b001 : 
            (num_samples <= 108) ? 0b000 : 0b111;
  if (num_samples > 108) return 6; // Quá nhiều mẫu yêu cầu

  ADS1115_read_r0(my_ADS1115, adrrDevice, channel);
  #if ADS1115_DEBUG
    printf("vontage_ref: %d.%02d V\n", (int)(my_ADS1115->vontage_ref), (int)((my_ADS1115->vontage_ref - (int)(my_ADS1115->vontage_ref)) * 100));
    printf("Number of samples required for A0 GND: %u\n", num_samples);
  #endif

  return 0;
}