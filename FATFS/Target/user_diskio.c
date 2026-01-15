/* USER CODE BEGIN Header */
/**
 ******************************************************************************
  * @file    user_diskio.c
  * @brief   This file includes a diskio driver skeleton to be completed by the user.
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

#ifdef USE_OBSOLETE_USER_CODE_SECTION_0
/*
 * Warning: the user section 0 is no more in use (starting from CubeMx version 4.16.0)
 * To be suppressed in the future.
 * Kept to ensure backward compatibility with previous CubeMx versions when
 * migrating projects.
 * User code previously added there should be copied in the new user sections before
 * the section contents can be deleted.
 */
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */
#endif

/* USER CODE BEGIN DECL */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "ff_gen_drv.h"
#include "user_diskio.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
const uint8_t dummy[SD_DATA_LEN+4] = {0xff};
#if SD_DEBUG_ON
	const char *error_names[] = {
		"SPI not respone",                         // 0: unused
		"Card idle state R1/7",           // 1
		"Illegal command R1/7",           // 2
		"Communication CRC error R1/7",   // 3
		"Erase sequence error R1/7",      // 4
		"Address error R1/7",             // 5
		"Parameter error R1/7",            // 6
		"check pattern error R7",			//7
		"voltage range error R7",			//8
		"error in position 9",				//9
		"error in position 10",				//10
		"error in position 11: card not support this VDD",				//11
		"error in position 12",
		"Unusable Card",
		"error in position 14",
		"CARD support: Reserved for Low Voltage Range",	//15
		"CARD support VDD: 2.7-2.8",
		"CARD support VDD: 2.8-2.9",
		"CARD support VDD: 2.9-3.0",
		"CARD support VDD: 3.0-3.1",
		"CARD support VDD: 3.1-3.2",
		"CARD support VDD: 3.2-3.3",
		"CARD support VDD: 3.3-3.4",
		"CARD support VDD: 3.4-3.5",
		"CARD support VDD: 3.5-3.6",						//24
		"error in position 25",
		"Card power is busy",
		"This device is not support CARD MMC", //27
		"ERROR READ DATA",									//28
		"CC ERROR READ DATA",								//29
		"CARD ECC FALILED READ DATA",						//30
		"OUT OF RANGE READ DATA",							//31
		"Data rejected due to a CRC error",					//32
		"Data Rejected due to a Write Error",				//33
		"error in position 34"								//34
	};
#endif
/* Private variables ---------------------------------------------------------*/
#if SD_DEBUG_ON
void SD_print_error_mess(SD_handle *mySD_handle){
    bool has_error = 0;
    for(uint8_t i = 0; i < SD_quantity_mess; i++){
        uint8_t e = mySD_handle->error_mess[i];
        if(e != 0 && e < sizeof(error_names)/sizeof(error_names[0])){
            printf("Error %d: %s\n", e, error_names[e]); // in e thay vì i
            has_error = 1;
        }
    }
    if(!has_error) printf("no error\n");

    // Reset error
    for(uint8_t i = 0; i < SD_quantity_mess; i++)
        mySD_handle->error_mess[i] = 0;
    printf("\n\n");
}


void SD_push_error_mess(uint8_t error, SD_handle *mySD_handle )
{
	for(uint8_t i = 0;i < SD_quantity_mess;i++ ){
		if(mySD_handle->error_mess[i] == error) break;
		if(mySD_handle->error_mess[i] == 0){
			mySD_handle->error_mess[i] = error;
			break;
		}
	}
}




void SD_check_R1(uint8_t data, SD_handle *mySD_handle){
    if(data & 0x01) SD_push_error_mess(1, mySD_handle); // IDLE_STATE
    if(data & 0x04) SD_push_error_mess(2, mySD_handle); // ILLEGAL_COMMAND
    if(data & 0x08) SD_push_error_mess(3, mySD_handle); // COM_CRC_ERROR
    if(data & 0x10) SD_push_error_mess(4, mySD_handle); // ERASE_SEQ_ERROR
    if(data & 0x20) SD_push_error_mess(5, mySD_handle); // ADDR_ERROR
    if(data & 0x40) SD_push_error_mess(6, mySD_handle); // PARAM_ERROR
}


void SD_check_R7(const uint8_t *data, SD_handle *mySD_handle){
    // Kiểm tra R1
    SD_check_R1(data[0], mySD_handle);

    // Kiểm tra check pattern
    if(data[4] != 0xAA){  // cuối cùng là check pattern
        SD_push_error_mess(7, mySD_handle); // custom error
    }

    // Kiểm tra VHS
    if((data[3] & 0x0F) != 0x01){  // voltage range không đúng
        SD_push_error_mess(8, mySD_handle); // custom error
    }
}
void SD_check_R3(const uint8_t *data, SD_handle *mySD_handle){
    // Kiểm tra R1
    SD_check_R1(data[0], mySD_handle);

    // custom error
    if(data[4] & 0x40) SD_push_error_mess(15, mySD_handle);
    if(data[3] & 0x08) SD_push_error_mess(16, mySD_handle);
    if(data[2] & 0x01) SD_push_error_mess(17, mySD_handle);
    if(data[2] & 0x02) SD_push_error_mess(18, mySD_handle);
    if(data[2] & 0x04) SD_push_error_mess(19, mySD_handle);
    if(data[2] & 0x08) SD_push_error_mess(20, mySD_handle);
    if(data[2] & 0x10) SD_push_error_mess(21, mySD_handle);
    if(data[2] & 0x20) SD_push_error_mess(22, mySD_handle);
    if(data[2] & 0x40) SD_push_error_mess(23, mySD_handle);
    if(data[2] & 0x80) SD_push_error_mess(24, mySD_handle);
    if(!(data[1] & 0x80)) SD_push_error_mess(26, mySD_handle);
}

void SD_check_SD_data_token_err(const uint8_t *data,SD_handle *mySD_handle){
	if(data[0]&0b00000001) SD_push_error_mess(28, mySD_handle);
	if(data[0]&0b00000010) SD_push_error_mess(29, mySD_handle);
	if(data[0]&0b00000100) SD_push_error_mess(30, mySD_handle);
	if(data[0]&0b00001000) SD_push_error_mess(31, mySD_handle);
}

void SD_check_SD_data_respone_token(const uint8_t *data,SD_handle *mySD_handle){
	if((mySD_handle->data[0]&0b1110) == 0b1010) SD_push_error_mess(32, mySD_handle);
	if((mySD_handle->data[0]&0b1110) == 0b1100) SD_push_error_mess(33, mySD_handle);
}

#endif

bool SD_SPI_rx_multi(SD_handle *mySD_handle,uint16_t len)
{
	uint16_t time_out = SD_TIME_OUT_SPI;

		mySD_handle->is_spi_rx_ready = 0;
		HAL_SPI_TransmitReceive_DMA(&hspi1, dummy, mySD_handle->data, len);

		do{
			SD_WAIT_SPI(1);
			if(mySD_handle->is_spi_rx_ready )break;
		}while(--time_out);
		if(!time_out)return 0;



	return 1;
}

bool SD_SPI_tx_multi(SD_handle *mySD_handle, uint16_t len)
{

	uint16_t time_out = SD_TIME_OUT_SPI;

		mySD_handle->is_spi_tx_ready = 0;

		HAL_SPI_Transmit_DMA(&hspi1, mySD_handle->data, len);

		do{
				SD_WAIT_SPI(1);
				if(mySD_handle->is_spi_tx_ready) break;
			}while(--time_out);
		if(!time_out)return 0;

	return 1;

}

bool SD_transmit_dummy(){
	uint16_t time_out = SD_TIME_OUT_SPI;

	if (HAL_SPI_Transmit(&hspi1, dummy , 1, time_out) == HAL_OK)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


uint8_t SD_crc7(uint8_t cmd, uint32_t arg) {
    uint64_t data = ((uint64_t)(cmd | 0x40) << 32) | arg; // ghép CMD + ARG
    uint8_t crc = 0;
    uint8_t poly = 0x09; // đa thức CRC7: x^7 + x^3 + 1

    // Tính CRC7 bit theo bit
    for (int i = 39; i >= 0; i--) {
        uint8_t bit = (data >> i) & 1;
        uint8_t c = (crc >> 6) & 1; // MSB CRC
        crc <<= 1;
        if (bit ^ c) {
            crc ^= poly;
        }
        crc &= 0x7F; // chỉ giữ 7 bit
    }

    return crc;
}

uint16_t SD_crc16(const uint8_t *pData, size_t len) {
    uint16_t crc = 0;           // Giá trị khởi tạo là 0 cho SD Data Block
    uint16_t poly = 0x1021;     // Đa thức Generator: x^16 + x^12 + x^5 + 1
                                // Binary: 1 0001 0000 0010 0001 (Bỏ bit cao nhất) -> 0x1021

    for (size_t i = 0; i < len; i++) {
        // Đưa byte dữ liệu hiện tại vào vị trí cao nhất của CRC để xử lý
        crc ^= ((uint16_t)pData[i] << 8);

        // Xử lý từng bit trong byte (8 bit)
        for (uint8_t j = 0; j < 8; j++) {
            // Kiểm tra bit MSB (bit 15)
            if (crc & 0x8000) {
                // Nếu bit MSB là 1: Dịch trái 1 và XOR với đa thức
                crc = (crc << 1) ^ poly;
            } else {
                // Nếu bit MSB là 0: Chỉ dịch trái 1
                crc <<= 1;
            }
        }
    }

    return crc;
}

bool SD_transmit_command(uint8_t cmd, uint32_t arg,SD_handle *mySD_handle)
{
    // transmit command to sd card
	memset(mySD_handle->data, 0xFF, 6);

	mySD_handle->data[0] = (cmd|0x40);
    // transmit argument
	mySD_handle->data[1] = ((uint8_t)(arg >> 24));
	mySD_handle->data[2] = ((uint8_t)(arg >> 16));
	mySD_handle->data[3] = ((uint8_t)(arg >> 8));
	mySD_handle->data[4] = ((uint8_t)(arg));
	mySD_handle->data[5] = 0;
	if((cmd==SD_CMD0)||(cmd==SD_CMD8)) mySD_handle->data[5] = (SD_crc7(cmd, arg)<<1)|1;
	#if SD_DEBUG_ON
		printf("command send CMD%d : ",cmd);
		for(uint8_t i=0;i<6;i++) printf("%02x ",mySD_handle->data[i]);
		printf("\n");
	#endif

	if(SD_SPI_tx_multi(mySD_handle, 6))return 1;
	else return 0;
}


bool SD_receive_command(SD_handle *mySD_handle, uint8_t typeResponse, uint8_t *respone)
{
	uint16_t time_out=SD_TIME_OUT_RESPONE_SD;
	if(typeResponse == SD_R1){
		memset(mySD_handle->data, 0xFF, 1);
	    while (--time_out){
	    	if(!SD_SPI_rx_multi(mySD_handle, 1))return 0;
	    	if (mySD_handle->data[0] != 0xFF) break;
	    	SD_WAIT(1);
	    }
	    if (!time_out) return 0;

	    respone[0] = mySD_handle->data[0];

		#if SD_DEBUG_ON
	    	SD_check_R1(respone[0], mySD_handle);
			printf("This is respone from R1: ");
			printf("%02x ",respone[0]);
			printf("\n");
		#endif

		return 1;
	}

	else if(typeResponse == SD_R7 || typeResponse == SD_R3){
		memset(mySD_handle->data, 0xFF, 5);

	    // Nhận byte R7 đầu tiên
	    while(--time_out){
	    	 if(!SD_SPI_rx_multi( mySD_handle, 1))return 0;
	    	 if(mySD_handle->data[0] != 0xFF) break;

	    }
	    if (!time_out) return 0;

	    respone[0] = mySD_handle->data[0];

	    if(respone[0] != 0xFF) if(!SD_SPI_rx_multi( mySD_handle, 4)) return 0; // Nhận 4 byte còn lại

	    // Copy dữ liệu ra response ngoài
	    for(int i=1; i<(5); i++){
	        respone[i] = mySD_handle->data[i-1];
	    }

		#if SD_DEBUG_ON
	    	if(typeResponse == SD_R7){
	    		SD_check_R7(respone, mySD_handle);
	    		printf("This is respone from R7: ");
	    	}else{
	    		SD_check_R3(respone, mySD_handle);
	    		printf("This is respone from R3: ");
	    	}

			for(uint8_t i=0;i<5;i++) printf("%02x ",respone[i]);
			printf("\n");
		#endif

		return 1;
	}
	return 0;

}

void print_hex_512(uint8_t *p)
{
    for (int i = 0; i < 512; i++) {
    	HAL_Delay(1);
        printf("%02X ", p[i]);
        if ((i + 1) % 16 == 0) printf("\n");  // xuống dòng mỗi 16 byte
    }
}

bool SD_receive_data(SD_handle *mySD_handle, uint32_t data_len,uint8_t *buff)
{
	uint16_t time_out = SD_TIME_OUT_RESPONE_SD;
	if(data_len==1){

		memset(mySD_handle->data, 0xFF, SD_DATA_LEN+2);
		do {
			if(!SD_SPI_rx_multi( mySD_handle, 1)) return 0;
			if (mySD_handle->data[0]!=0xff)break;
		}while (--time_out);


		#if SD_DEBUG_ON
			if(!(mySD_handle->data[0]&0b11110000)) SD_check_SD_data_token_err(mySD_handle->data,mySD_handle);
			printf("This is respone from first byte data: ");
			printf("%02x ",mySD_handle->data[0]);
			printf("\n");
		#endif

		if(!time_out) return 0;

		if(mySD_handle->data[0]!=0b11111110){
			return 0;
		}

		if(!SD_SPI_rx_multi( mySD_handle, SD_DATA_LEN+2)) return 0;

		memcpy(buff, mySD_handle->data, SD_DATA_LEN);
		#if SD_DEBUG_ON
			print_hex_512(buff);
		#endif
		time_out = SD_TIME_OUT_SPI;
		for(uint8_t i=0;i<time_out;i++) SD_transmit_dummy();



	}else{
		memset(mySD_handle->data, 0xFF, SD_DATA_LEN+3);
		do{
			time_out = SD_TIME_OUT_RESPONE_SD;
			memset(mySD_handle->data, 0xFF, 1);
			while (--time_out){
				SD_SPI_rx_multi( mySD_handle, 1);
				if (mySD_handle->data[0]!=0xff)break;
				SD_WAIT(1);
			}

			#if SD_DEBUG_ON
				if(!(mySD_handle->data[0]&0b1111000)) SD_check_SD_data_token_err(mySD_handle->data,mySD_handle);
				printf("This is respone from first byte data: ");
				printf("%02x ",mySD_handle->data[0]);
				printf("\n");
			#endif

			if(!time_out) return 0;

			if(mySD_handle->data[0]!=0b11111100){


				if(!(mySD_handle->data[0]&0b1111000)){
					#if SD_DEBUG_ON
						
					SD_check_SD_data_token_err(mySD_handle->data,mySD_handle);
					#endif
				} 
				return 0;
			}

			if(!SD_SPI_rx_multi( mySD_handle, SD_DATA_LEN+3))return 0;

			if(mySD_handle->data[SD_DATA_LEN+3]!=0b11111101) {
				return 0;
			}
			memcpy(buff, mySD_handle->data, SD_DATA_LEN);
			buff+=SD_DATA_LEN;
			time_out = SD_TIME_OUT_SPI;
			for(uint8_t i=0;i<time_out;i++) SD_transmit_dummy();

		}while(--data_len);

	}
	return 1;

}

bool SD_transmit_data(SD_handle *mySD_handle, uint32_t data_len,const uint8_t *buff)
{
	uint16_t time_out;

	if(data_len ==1)
	{
		time_out = SD_TIME_OUT_RESPONE_SD;
		memset(mySD_handle->data, 0xFF, SD_DATA_LEN+2);
		mySD_handle->data[0]=0b11111110;
		if(!SD_SPI_tx_multi( mySD_handle, 1))return 0;
		memcpy(mySD_handle->data,buff,SD_DATA_LEN);
		uint16_t temp = SD_crc16(buff, 512);

		// Sửa lỗi: Gán thủ công để đảm bảo Big Endian (MSB trước, LSB sau)
		mySD_handle->data[SD_DATA_LEN]     = (uint8_t)(temp >> 8);   // Byte cao (Bits 15-8)
		mySD_handle->data[SD_DATA_LEN + 1] = (uint8_t)(temp & 0xFF); // Byte thấp (Bits 7-0)

		if(!SD_SPI_tx_multi( mySD_handle, SD_DATA_LEN+2))return 0;
		do{
			if(!SD_SPI_rx_multi( mySD_handle, 1))return 0;
			if(mySD_handle->data[0]!=0xff) break;
		}while(--time_out);

		#if SD_DEBUG_ON
			if((mySD_handle->data[0]&0b1110) != 0b0100) SD_check_SD_data_respone_token(mySD_handle->data,mySD_handle);
			printf("This is respone from %lu transmit data: ",data_len);
			printf("%02x ",mySD_handle->data[0]);
			printf("\n");
		#endif
		if((mySD_handle->data[0]&0b1110) != 0b0100)return 0;
		time_out = SD_TIME_OUT_RESPONE_SD;
		do{
			if(!SD_SPI_rx_multi( mySD_handle, 1))return 0;
			if(mySD_handle->data[0]== 0xff) break;
		}while(--time_out);
		#if SD_DEBUG_ON
			if(!time_out) printf("error in position: auwdjsn error: time out \n");
		#endif

		for(uint8_t i=0;i<15;i++) SD_transmit_dummy();

		if(!time_out) return 0;

	}else{
		time_out = SD_TIME_OUT_RESPONE_SD;
		memset(mySD_handle->data, 0xFF, SD_DATA_LEN+2);
		mySD_handle->data[0]=0b11111100;
		if(!SD_SPI_tx_multi( mySD_handle, 1))return 0;
		memcpy(mySD_handle->data,buff,SD_DATA_LEN);
		uint16_t temp = SD_crc16(buff, 512);

		// Sửa lỗi: Gán thủ công để đảm bảo Big Endian (MSB trước, LSB sau)
		mySD_handle->data[SD_DATA_LEN]     = (uint8_t)(temp >> 8);   // Byte cao (Bits 15-8)
		mySD_handle->data[SD_DATA_LEN + 1] = (uint8_t)(temp & 0xFF); // Byte thấp (Bits 7-0)

		if(!SD_SPI_tx_multi( mySD_handle, SD_DATA_LEN+2))return 0;
		do{
			if(!SD_SPI_rx_multi( mySD_handle, 1))return 0;
			if(mySD_handle->data[0]!=0xff) break;
		}while(--time_out);
		#if SD_DEBUG_ON
			SD_check_SD_data_respone_token(mySD_handle->data, mySD_handle);
			if((mySD_handle->data[0]&0b1110)!=0b0100)SD_push_error_mess(SD_POSITION_34, mySD_handle);
		#endif

		if ((mySD_handle->data[0] & 0b1110) != 0b0100) return 0;
		time_out = SD_TIME_OUT_RESPONE_SD;
		do{
			if(!SD_SPI_rx_multi( mySD_handle, 1))return 0;
			if(mySD_handle->data[0]==0xff) break;
		}while(--time_out);
		#if SD_DEBUG_ON
			if(!time_out)printf("error in position: wqeqw212 error: time out \n");
		#endif
		for(uint8_t i=0;i<15;i++) SD_transmit_dummy();


		if(!time_out) return 0;
	}

	return 1;
}






bool SD_receive_CSD(SD_handle *mySD_handle){
	uint16_t time_out = SD_TIME_OUT_RESPONE_SD;
	memset(mySD_handle->data, 0xFF, 16);
	do{
		if(!SD_SPI_rx_multi( mySD_handle, 1))return 0;
		if(mySD_handle->data[0]!=0xff) break;
	}while(--time_out);
	if(mySD_handle->data[0]!=0xFE) return 0;
	if(!SD_SPI_rx_multi( mySD_handle, 16))return 0;
	#if SD_DEBUG_ON
		printf("this is data from SCD: ");
		for (int i = 0; i < 16; i++) {
			HAL_Delay(1);
			printf("%02X ", mySD_handle->data[i]);
			if ((i + 1) % 16 == 0) printf("\n");  // xuống dòng mỗi 16 byte
		}
		printf("/n");
	#endif
	SD_transmit_dummy();
	SD_transmit_dummy();
	SD_transmit_dummy();
	SD_transmit_dummy();
	return 1;
}

void SD_read_CSD(const uint8_t *csd, SD_info *info)
{
    // ------------------------------
    // Kiểm tra loại CSD
    // ------------------------------
    uint8_t csd_structure = (csd[0] >> 6) & 0x03;

    if (csd_structure == 1)
    {
        // =====================================================
        // SDHC / SDXC (CSD Version 2.0)
        // =====================================================
        uint32_t c_size =
            ((uint32_t)(csd[7] & 0x3F) << 16) |
            ((uint32_t)csd[8] << 8) |
             (uint32_t)csd[9];

        info->block_size   = 512;                   // byte
        info->block_count  = (c_size + 1) * 1024;   // số block
        info->capacity_mb  = ((uint64_t)info->block_count * 512ULL) / (1024 * 1024); // MB
    }
    else
    {
        // =====================================================
        // SDSC (<= 2GB) – CSD Version 1.0
        // =====================================================
        uint32_t c_size =
            ((uint32_t)(csd[6] & 0x03) << 10) |
            ((uint32_t) csd[7]        <<  2) |
            ((uint32_t)(csd[8] >> 6)  & 0x03);

        uint8_t c_size_mult =
            ((csd[9]  & 0x03) << 1) |
            ((csd[10] >> 7)    & 0x01);

        uint8_t read_bl_len = csd[5] & 0x0F;

        info->block_size   = 1UL << read_bl_len;   // byte
        info->block_count  = (c_size + 1) * (1UL << (c_size_mult + 2));
        info->capacity_mb  = ((uint64_t)info->block_size * info->block_count) / (1024 * 1024);
    }

    // ================================
    // Giải mã TRAN_SPEED (tốc độ tối đa)
    // ================================
    uint8_t ts = csd[3];
    uint8_t mant = ts & 0x0F;
    uint8_t exp  = (ts >> 3) & 0x07;

    static const uint16_t mant_table[16] = {
         0,10,12,13,15,20,25,30,35,40,45,50,55,60,70,80
    };

    // Speed = mantissa × 10^exp kbit/s → đổi sang Hz
    info->max_speed_hz = (uint32_t)mant_table[mant] * (100000UL << exp);
}



void SD_Set_SPI_Speed(uint32_t prescaler)
{
    // 1. Phải vô hiệu hóa SPI trước khi đổi cấu hình
    __HAL_SPI_DISABLE(&hspi1);

    // 2. Cập nhật Prescaler mới vào Struct Init
    hspi1.Init.BaudRatePrescaler = prescaler;

    // 3. Gọi lại hàm Init để áp dụng thay đổi vào thanh ghi phần cứng
    // HAL_SPI_Init sẽ tự động xử lý các cờ và bật lại SPI nếu cần
    if (HAL_SPI_Init(&hspi1) != HAL_OK)
    {
        // Xử lý lỗi nếu cần (hoặc gọi Error_Handler)
        // Error_Handler();
    }

    // 4. Đảm bảo SPI được bật lại (thường HAL_SPI_Init đã bật, nhưng thêm cho chắc)
    __HAL_SPI_ENABLE(&hspi1);
}




/* Disk status */
static volatile DSTATUS Stat = STA_NOINIT;

/* USER CODE END DECL */

/* Private function prototypes -----------------------------------------------*/
DSTATUS USER_initialize (BYTE pdrv);
DSTATUS USER_status (BYTE pdrv);
DRESULT USER_read (BYTE pdrv, BYTE *buff, DWORD sector, UINT count);
#if _USE_WRITE == 1
  DRESULT USER_write (BYTE pdrv, const BYTE *buff, DWORD sector, UINT count);
#endif /* _USE_WRITE == 1 */
#if _USE_IOCTL == 1
  DRESULT USER_ioctl (BYTE pdrv, BYTE cmd, void *buff);
#endif /* _USE_IOCTL == 1 */

Diskio_drvTypeDef  USER_Driver =
{
  USER_initialize,
  USER_status,
  USER_read,
#if  _USE_WRITE
  USER_write,
#endif  /* _USE_WRITE == 1 */
#if  _USE_IOCTL == 1
  USER_ioctl,
#endif /* _USE_IOCTL == 1 */
};

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Initializes a Drive
  * @param  pdrv: Physical drive number (0..)
  * @retval DSTATUS: Operation status
  */
DSTATUS USER_initialize (
	BYTE pdrv           /* Physical drive nmuber to identify the drive */
)
{
  /* USER CODE BEGIN INIT */
	// 80 clock cycles để khởi động SD card
		memset(mySD_handle->data,0xff,512);
		SD_CS_HIGH();
		for (uint8_t i = 0; i < 10; i++) SD_SPI_tx_multi(mySD_handle, 1);

		uint8_t responeCMD0[SD_R1];
		responeCMD0[0] = 0xff;

		SD_transmit_dummy();
		SD_CS_LOW();
		SD_transmit_dummy();


		SD_transmit_command(SD_CMD0, 0, mySD_handle);
		SD_receive_command(mySD_handle, SD_R1, responeCMD0);

		SD_transmit_dummy();
		SD_CS_HIGH();
		SD_transmit_dummy();

		if(responeCMD0[0] != 0x01){
			#if SD_DEBUG_ON
				SD_push_error_mess(SD_POSITION_9, mySD_handle);
				SD_push_error_mess(0, mySD_handle);
			SD_push_error_mess(SD_POSITION_9, mySD_handle);
			SD_push_error_mess(0, mySD_handle);
			#endif
			return STA_NODISK;
		}
		uint32_t arg = 0b0001;//voltage supplied (VHS)
		arg =(arg<<8)|0xAA; //checkpattern
		//biến chứa phản hồi
		uint8_t responeCMD8[SD_R7-2]={0};
		SD_transmit_dummy();
		SD_CS_LOW();
		SD_transmit_dummy();

		SD_transmit_command(SD_CMD8, arg, mySD_handle);
		SD_receive_command(mySD_handle, SD_R7, responeCMD8);


		SD_transmit_dummy();
		SD_CS_HIGH();
		SD_transmit_dummy();

		if(responeCMD8[0]&0x4) mySD_handle->is_MMC=1;
		else
		{
			if(responeCMD8[4] != 0xAA){  // cuối cùng là check pattern
				#if SD_DEBUG_ON
					SD_push_error_mess(SD_POSITION_10, mySD_handle);
					SD_push_error_mess(13, mySD_handle);
				#endif
				return STA_NOINIT;
			}
			if((responeCMD8[3] & 0x0F) != 0b0001){  // voltage range không đúng
				#if SD_DEBUG_ON
					SD_push_error_mess(SD_POSITION_11, mySD_handle);
					SD_push_error_mess(13, mySD_handle);
				#endif
				// send CMD58
				arg = 0;
				uint8_t responeCMD58[SD_R3+2]={0xff};

				SD_transmit_dummy();
				SD_CS_LOW();
				SD_transmit_dummy();

				SD_transmit_command(SD_CMD58, arg, mySD_handle);
				SD_receive_command(mySD_handle, SD_R3, responeCMD58);

				SD_transmit_dummy();
				SD_CS_HIGH();
				SD_transmit_dummy();
				return STA_NOINIT;
			 }
		}
		arg = 0x01  <<  30;

		uint8_t responeCMD55[SD_R1]={0xff};
		uint8_t responeCMD41[SD_R1]={0xff};
		uint8_t responeCMD58[SD_R3+2]={0xff};
		uint16_t time_out=200;
		//Gửi ACMD41 + CMD58
		do{
			responeCMD55[0]=0xff;
			responeCMD41[0]=0xff;
			responeCMD58[0]=0xff;

			SD_transmit_dummy();
			SD_CS_LOW();
			SD_transmit_dummy();

			SD_transmit_command(SD_CMD55, 0 , mySD_handle);
			SD_receive_command(mySD_handle, SD_R1, responeCMD55);

			SD_transmit_command(SD_CMD41, arg , mySD_handle);
			SD_receive_command(mySD_handle, SD_R1, responeCMD41);

			if (!mySD_handle->is_MMC){
				SD_transmit_command(SD_CMD58, 0, mySD_handle);
				SD_receive_command(mySD_handle, SD_R3, responeCMD58);
			}
			else responeCMD58[1] = 0x00;


			SD_transmit_dummy();
			SD_CS_HIGH();
			SD_transmit_dummy();

			if((responeCMD41[0] == 0x00)&& (responeCMD58[1] != 0x00)){
				mySD_handle->is_card_ready=1;
				break;
			}

			SD_WAIT(10);

		}while(--time_out);

		if(!time_out) return STA_NOINIT;

		if (responeCMD58[1] & 0b01000000) mySD_handle->is_SDHC=1;
		else mySD_handle->is_SDSC=1;

		if (mySD_handle->is_MMC || mySD_handle->is_SDSC){

			uint8_t responeCMD16[SD_R1]={0xff};


			SD_transmit_dummy();
			SD_CS_LOW();
			SD_transmit_dummy();

			SD_transmit_command(SD_CMD16, 512, mySD_handle);
			SD_receive_command(mySD_handle, SD_R1, responeCMD16);

			SD_transmit_dummy();
			SD_CS_HIGH();
			SD_transmit_dummy();
		}
		uint8_t responeCMD9[SD_R1]={0xff};
		SD_transmit_dummy();
		SD_CS_LOW();
		SD_transmit_dummy();

		SD_transmit_command(SD_CMD9, 0, mySD_handle);
		SD_receive_command(mySD_handle, SD_R1, responeCMD9);
		SD_receive_CSD(mySD_handle);

		SD_transmit_dummy();
		SD_CS_HIGH();
		SD_transmit_dummy();

		SD_read_CSD(mySD_handle->data, &mySD_handle->info);

		#if SD_DEBUG_ON
		printf("Dung luong: %lu mb\n", mySD_handle->info.capacity_mb);
		printf("So block: %lu\n", mySD_handle->info.block_count);
		printf("Block size: %lu\n", mySD_handle->info.block_size);
		printf("Baund rate: %lu\n",mySD_handle->info.max_speed_hz);
		SD_Set_SPI_Speed(SD_BAUND_RATE);
		#endif
		return 0; // OK
  /* USER CODE END INIT */
}

/**
  * @brief  Gets Disk Status
  * @param  pdrv: Physical drive number (0..)
  * @retval DSTATUS: Operation status
  */
DSTATUS USER_status (
	BYTE pdrv       /* Physical drive number to identify the drive */
)
{
  /* USER CODE BEGIN STATUS */
    return 0;
  /* USER CODE END STATUS */
}

/**
  * @brief  Reads Sector(s)
  * @param  pdrv: Physical drive number (0..)
  * @param  *buff: Data buffer to store read data
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to read (1..128)
  * @retval DRESULT: Operation result
  */
DRESULT USER_read (
	BYTE pdrv,      /* Physical drive nmuber to identify the drive */
	BYTE *buff,     /* Data buffer to store read data */
	DWORD sector,   /* Sector address in LBA */
	UINT count      /* Number of sectors to read */
)
{
  /* USER CODE BEGIN READ */
	if (pdrv != 0) return RES_PARERR;
	if (count == 0) return RES_PARERR;
	if (!mySD_handle->is_card_ready) return RES_PARERR;
	// SDSC dùng byte-addressing
	if (!mySD_handle->is_SDHC) sector <<= 9;   // sector * 512
	bool res=0;
	if(count == 1){

		uint8_t responeCMD17[SD_R1]={0xff};

		SD_transmit_dummy();
		SD_CS_LOW();
		SD_transmit_dummy();

		SD_transmit_command(SD_CMD17, sector , mySD_handle);
		SD_receive_command(mySD_handle, SD_R1, responeCMD17);
		if(responeCMD17[0] == 0x00) res = SD_receive_data(mySD_handle,count,buff);

		SD_transmit_dummy();
		SD_CS_HIGH();
		SD_transmit_dummy();

		if(responeCMD17[0] != 0x00)return RES_ERROR;
		if(!res) return RES_ERROR;

	}else{
		uint8_t responeCMD18[SD_R1]={0xff};
		uint8_t responeCMD12[SD_R1b]={0xff};

		SD_transmit_dummy();
		SD_CS_LOW();
		SD_transmit_dummy();

		SD_transmit_command(SD_CMD18, sector , mySD_handle);
		SD_receive_command(mySD_handle, SD_R1, responeCMD18);
		if(responeCMD18[0]==0x00) {
			res = SD_receive_data(mySD_handle,count,buff);

			SD_transmit_command(SD_CMD12, 0 , mySD_handle);
			SD_receive_command(mySD_handle, SD_R1, responeCMD12);
		}

		SD_transmit_dummy();
		SD_CS_HIGH();
		SD_transmit_dummy();

		if(responeCMD18[0] != 0x00)return RES_ERROR;
		if(responeCMD12[0] != 0x00)return RES_ERROR;
		if(!res) return RES_ERROR;
	}
    return RES_OK;
  /* USER CODE END READ */
}

/**
  * @brief  Writes Sector(s)
  * @param  pdrv: Physical drive number (0..)
  * @param  *buff: Data to be written
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to write (1..128)
  * @retval DRESULT: Operation result
  */
#if _USE_WRITE == 1
DRESULT USER_write (
	BYTE pdrv,          /* Physical drive nmuber to identify the drive */
	const BYTE *buff,   /* Data to be written */
	DWORD sector,       /* Sector address in LBA */
	UINT count          /* Number of sectors to write */
)
{
  /* USER CODE BEGIN WRITE */
  /* USER CODE HERE */
	if (pdrv != 0) return RES_PARERR;  // chỉ hỗ trợ drive 0
	if (!mySD_handle->is_card_ready) return RES_NOTRDY;
	if (count == 0) return RES_PARERR;
	// SDSC dùng byte addressing
	if (!mySD_handle->is_SDHC) sector <<= 9;   // sector * 512
	bool res=0;
	if(count == 1){

		uint8_t responeCMD24[SD_R1]={0xff};


		SD_transmit_dummy();
		SD_CS_LOW();
		SD_transmit_dummy();

		SD_transmit_command(SD_CMD24, sector , mySD_handle);
		SD_receive_command(mySD_handle, SD_R1, responeCMD24);
		if(responeCMD24[0]== 0x00) res = SD_transmit_data(mySD_handle,count,buff);

		SD_transmit_dummy();
		SD_CS_HIGH();
		SD_transmit_dummy();

		if(responeCMD24[0]!= 0x00) return RES_ERROR;
		if(!res) return RES_ERROR;
	}else{
		uint8_t responeCMD25[SD_R1]={0xff};
		uint8_t responeCMD12[SD_R1b]={0xff};

		SD_transmit_dummy();
		SD_CS_LOW();
		SD_transmit_dummy();

		SD_transmit_command(SD_CMD25, sector , mySD_handle);
		SD_receive_command(mySD_handle, SD_R1, responeCMD25);
		if(responeCMD25[0]==0x00){
			res = SD_transmit_data(mySD_handle,count,buff);
			SD_transmit_command(SD_CMD12, 0 , mySD_handle);
			SD_receive_command(mySD_handle, SD_R1, responeCMD12);
		}

		SD_transmit_dummy();
		SD_CS_HIGH();
		SD_transmit_dummy();

		if(responeCMD25[0]!= 0x00) return RES_ERROR;
		if(responeCMD12[0]!= 0x00) return RES_ERROR;
		if(!res) return RES_ERROR;
	}

    return RES_OK;
  /* USER CODE END WRITE */
}
#endif /* _USE_WRITE == 1 */

/**
  * @brief  I/O control operation
  * @param  pdrv: Physical drive number (0..)
  * @param  cmd: Control code
  * @param  *buff: Buffer to send/receive control data
  * @retval DRESULT: Operation result
  */
#if _USE_IOCTL == 1
DRESULT USER_ioctl (
	BYTE pdrv,      /* Physical drive nmuber (0..) */
	BYTE cmd,       /* Control code */
	void *buff      /* Buffer to send/receive control data */
)
{
  /* USER CODE BEGIN IOCTL */
    DRESULT res = RES_ERROR;
    return res;
  /* USER CODE END IOCTL */
}
#endif /* _USE_IOCTL == 1 */

