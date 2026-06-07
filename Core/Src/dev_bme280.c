/*
 * @file 	dev_bmp280.c
 * @author  Mem 3 - Nguyễn Công Trường
 * @brief   Thư viện điều khiển cảm biến nhiệt độ/áp suất BMP280.
 */

#include "dev_bme280.h"
#include "hw_i2c.h"
#include "stdio.h"

/* ĐỊNH NGHĨA ĐỊA CHỈ & THANH GHI BÊN TRONG BMP280 */

/* ĐỊA CHỈ GIAO TIẾP I2C (BẮT BUỘC DỊCH TRÁI 1 BIT ĐỂ CHỨA BIT R/W) */
#define BMP280_I2C_W_ADDR  (BMP280_ADDR << 1)       /* 0xEC (Ghi) */
#define BMP280_I2C_R_ADDR  ((BMP280_ADDR << 1) | 1) /* 0xED (Đọc) */

/* ĐỊA CHỈ CÁC THANH GHI QUAN TRỌNG TỪ DATASHEET CẢM BIẾN */
#define BMP280_REG_ID         0xD0 /* Thanh ghi chứa mã định danh */
#define BMP280_REG_CTRL_MEAS  0xF4 /* Thanh ghi điều khiển: Bật/tắt chế độ Sleep, chỉnh độ phân giải lấy mẫu */
#define BMP280_REG_TEMP_MSB   0xFA /* Thanh ghi đầu tiên (Byte cao nhất - MSB) chứa dữ liệu nhiệt độ đo được */
#define BMP280_REG_CALIB_T1   0x88 /* Địa chỉ bắt đầu của khối nhớ ROM chứa các hằng số bù nhiệt */

/* BIẾN TOÀN CỤC CỤC BỘ LƯU HẰNG SỐ BÙ NHIỆT */
static uint16_t dig_T1;
static int16_t  dig_T2;
static int16_t  dig_T3;
static int32_t  t_fine;

/* CÁC HÀM HỖ TRỢ ĐỌC/GHI NỘI BỘ */

static void BMP280_Write_Reg(uint8_t reg, uint8_t value) {
	/* GHI 1 BYTE VÀO THANH GHI CỦA BMP280 */
	I2C1_Start();
	I2C1_WriteAddress(BMP280_I2C_W_ADDR);
	I2C1_WriteData(reg);    /* Gửi địa chỉ thanh ghi cần ghi */
	I2C1_WriteData(value);  /* Gửi giá trị cần cấu hình */
	I2C1_Stop();
}

static void BMP280_Read_Bytes(uint8_t reg, uint8_t *buffer, uint8_t length) {
	/* ĐỌC NHIỀU BYTE TỪ THANH GHI CỦA BMP280 */
	I2C1_Start();
	I2C1_WriteAddress(BMP280_I2C_W_ADDR);
	I2C1_WriteData(reg);    /* Trỏ tới thanh ghi cần đọc */

	I2C1_Start();           /* Repeated Start để đổi hướng truyền */
	I2C1_WriteAddress(BMP280_I2C_R_ADDR);

	/* ĐỌC DỮ LIỆU LIÊN TIẾP */
	for (uint8_t i = 0; i < length; i++) {
		if (i == length - 1) {
			buffer[i] = I2C1_ReadData_NACK(); /* Byte cuối trả NACK */
		} else {
			buffer[i] = I2C1_ReadData_ACK();  /* Các byte giữa trả ACK */
		}
	}
}

/* ĐỊNH NGHĨA CÁC HÀM CHÍNH                                                   */
void BMP280_Init(void) {
	uint8_t calib_data[6];
	uint8_t chip_id;

	/* ĐỌC MÃ CHIP ID ĐỂ KIỂM TRA KẾT NỐI (PHẢI BẰNG 0x58) */
	BMP280_Read_Bytes(BMP280_REG_ID, &chip_id, 1);
	if (chip_id != 0x58) {
		/* Nếu sai ID, có thể mạch lỏng hoặc hỏng, thoát hàm */
		printf("Lỗi kết nối");
		return;
	}

	/* ĐỌC 6 BYTE HẰNG SỐ BÙ NHIỆT (T1, T2, T3) TỪ THANH GHI 0x88 */
	BMP280_Read_Bytes(BMP280_REG_CALIB_T1, calib_data, 6);

	/* GÉP 2 BYTE THÀNH SỐ NGUYÊN */
	dig_T1 = (uint16_t)((calib_data[1] << 8) | calib_data[0]);
	dig_T2 = (int16_t) ((calib_data[3] << 8) | calib_data[2]);
	dig_T3 = (int16_t) ((calib_data[5] << 8) | calib_data[4]);

	/* CẤU HÌNH CẢM BIẾN (THANH GHI CTRL_MEAS REGISTER) */
	/* * Mode: Normal (Liên tục đo) -> 0x03
	 * Oversampling Temp (osrs_t): x1 (Độ phân giải 16-bit) -> 0x01
	 * Cấu hình byte: osrs_t[7:5] | osrs_p[4:2] | mode[1:0]
	 * Giá trị: (0x01 << 5) | (0x00 << 2) | 0x03 = 0x23
	 */
	BMP280_Write_Reg(BMP280_REG_CTRL_MEAS, 0x23);
}

float BMP280_Read_Temperature(void) {
	uint8_t raw_data[3];
	int32_t adc_T;
	int32_t var1, var2, T;

	/* ĐỌC 3 BYTE DỮ LIỆU NHIỆT ĐỘ (MSB, LSB, XLSB) */
	BMP280_Read_Bytes(BMP280_REG_TEMP_MSB, raw_data, 3);

	/* GÉP 3 BYTE THÀNH BIẾN ADC 20-BIT */
	adc_T = (int32_t)(((uint32_t)raw_data[0] << 12) |
	                  ((uint32_t)raw_data[1] << 4)  |
	                  ((uint32_t)raw_data[2] >> 4));

	/* CÔNG THỨC BÙ NHIỆT CỦA CẢM BIẾN */
	var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;

	var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;

	t_fine = var1 + var2;

	T = (t_fine * 5 + 128) >> 8;

	/* KẾT QUẢ T LÀ ĐỘ C NHÂN 100) */
	/* CHIA 100 ĐỂ TRẢ VỀ KIỂU FLOAT CHUẨN */
	return (float)T / 100.0f;
}
