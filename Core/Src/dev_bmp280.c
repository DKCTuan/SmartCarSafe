/**
 * @file    dev_bmp280.c
 * @author  Mem 3 - Nguyễn Công Trường
 * @brief   Thư viện điều khiển cảm biến nhiệt độ/áp suất BMP280.
 * Thực thi các hàm khởi tạo, đọc/ghi thanh ghi và tính toán nhiệt độ.
 */


#include "dev_bmp280.h"
#include "hw_i2c.h"
#include <stdio.h>


/* ĐỊA CHỈ GIAO TIẾP I2C (DỊCH TRÁI 1 BIT ĐỂ CHỨA BIT R/W) */
#define BMP280_I2C_W_ADDR     (BMP280_ADDR << 1)       /* 0xEC (Ghi) */
#define BMP280_I2C_R_ADDR     ((BMP280_ADDR << 1) | 1) /* 0xED (Đọc) */

/* ĐỊA CHỈ CÁC THANH GHI QUAN TRỌNG TỪ DATASHEET CẢM BIẾN */
#define BMP280_REG_ID         0xD0 /* Thanh ghi chứa mã định danh (Chip ID) */
#define BMP280_REG_RESET      0xE0 /* Thanh ghi Reset cảm biến */
#define BMP280_REG_CTRL_MEAS  0xF4 /* Thanh ghi điều khiển: Bật/tắt Sleep, độ phân giải */
#define BMP280_REG_TEMP_MSB   0xFA /* Thanh ghi chứa byte cao nhất (MSB) của nhiệt độ */
#define BMP280_REG_CALIB_T1   0x88 /* Địa chỉ bắt đầu khối ROM chứa hằng số bù nhiệt */

/* CÁC HỆ SỐ BÙ NHIỆT CỦA CẢM BIẾN */

static uint16_t dig_T1; /* Hệ số bù nhiệt T1 (Không dấu) */
static int16_t  dig_T2; /* Hệ số bù nhiệt T2 (Có dấu) */
static int16_t  dig_T3; /* Hệ số bù nhiệt T3 (Có dấu) */
static int32_t  t_fine; /* Biến lưu trữ nhiệt độ độ phân giải cao dùng nội bộ */


/**
 * @brief  Ghi 1 byte dữ liệu vào thanh ghi cấu hình của BMP280.
 * @param  I2Cx:  Con trỏ tới ngoại vi I2C (I2C1, I2C2,...).
 * @param  reg:   Địa chỉ thanh ghi cần ghi.
 * @param  value: Giá trị cần ghi vào thanh ghi.
 */
static void BMP280_Write_Reg(I2C_TypeDef *I2Cx, uint8_t reg, uint8_t value) {
    HW_I2C_Start(I2Cx);
    HW_I2C_WriteAddress(I2Cx, BMP280_I2C_W_ADDR);
    HW_I2C_WriteData(I2Cx, reg);    /* Gửi địa chỉ thanh ghi cần trỏ tới */
    HW_I2C_WriteData(I2Cx, value);  /* Gửi giá trị cấu hình */
    HW_I2C_Stop(I2Cx);
}

/**
 * @brief  Đọc chuỗi nhiều byte liên tiếp từ một thanh ghi bắt đầu của BMP280.
 * @param  I2Cx:   Con trỏ tới ngoại vi I2C (I2C1, I2C2,...).
 * @param  reg:    Địa chỉ thanh ghi bắt đầu đọc.
 * @param  buffer: Con trỏ mảng chứa dữ liệu trả về.
 * @param  length: Số lượng byte cần đọc.
 */
static void BMP280_Read_Bytes(I2C_TypeDef *I2Cx, uint8_t reg, uint8_t *buffer, uint8_t length) {
    /* Trỏ tới thanh ghi cần đọc */
    HW_I2C_Start(I2Cx);
    HW_I2C_WriteAddress(I2Cx, BMP280_I2C_W_ADDR);
    HW_I2C_WriteData(I2Cx, reg);

    /* Repeated Start để đổi hướng truyền sang chế độ Đọc */
    HW_I2C_Start(I2Cx);
    HW_I2C_WriteAddress(I2Cx, BMP280_I2C_R_ADDR);

    /* Đọc dữ liệu liên tiếp */
    for (uint8_t i = 0; i < length; i++) {
        if (i == (length - 1)) {
            buffer[i] = HW_I2C_ReadData_NACK(I2Cx); /* Byte cuối cùng gửi NACK */
        } else {
            buffer[i] = HW_I2C_ReadData_ACK(I2Cx);  /* Các byte giữa gửi ACK */
        }
    }
}

/**
 * @brief  Khởi tạo cảm biến BMP280, kiểm tra ID và tải cấu hình mặc định.
 * @param  I2Cx: Con trỏ tới ngoại vi I2C đang kết nối với cảm biến.
 */
void BMP280_Init(I2C_TypeDef *I2Cx) {
    uint8_t calib_data[6];
    uint8_t chip_id;

    /* 1. Kiểm tra mã định danh (Chip ID) để xác nhận kết nối */
    BMP280_Read_Bytes(I2Cx, BMP280_REG_ID, &chip_id, 1);
    if (chip_id != 0x58) {
        printf("Lỗi kết nối BMP280 trên đường I2C\n");
        return;
    }

    /* 2. Đọc 6 byte hằng số hiệu chuẩn nhiệt độ (T1, T2, T3) từ ROM */
    BMP280_Read_Bytes(I2Cx, BMP280_REG_CALIB_T1, calib_data, 6);

    /* 3. Ghép các byte thô thành số nguyên (Theo chuẩn Little-Endian) */
    dig_T1 = (uint16_t)((calib_data[1] << 8) | calib_data[0]);
    dig_T2 = (int16_t) ((calib_data[3] << 8) | calib_data[2]);
    dig_T3 = (int16_t) ((calib_data[5] << 8) | calib_data[4]);

    /* 4. Cấu hình chế độ hoạt động cho cảm biến (Thanh ghi CTRL_MEAS)
     * Mode: Normal (0x03)
     * Oversampling Temp (osrs_t): x1 (0x01)
     * Oversampling Press (osrs_p): Skipped (0x00)
     * Mã Hex tổng hợp: (0x01 << 5) | (0x00 << 2) | 0x03 = 0x23
     */
    BMP280_Write_Reg(I2Cx, BMP280_REG_CTRL_MEAS, 0x23);
}

/**
 * @brief  Hủy khởi tạo cảm biến BMP280 (Đưa về Sleep mode hoặc Soft Reset)
 * @param  I2Cx: Con trỏ tới ngoại vi I2C đang kết nối với cảm biến
 */
void BMP280_DeInit(I2C_TypeDef *I2Cx) {
    /* Gửi mã 0xB6 vào thanh ghi Reset để khôi phục toàn bộ cảm biến về mặc định */
    BMP280_Write_Reg(I2Cx, BMP280_REG_RESET, 0xB6);
}

/**
 * @brief  Đọc dữ liệu thô từ cảm biến và tính toán ra nhiệt độ chuẩn (Độ C).
 * @param  I2Cx: Con trỏ tới ngoại vi I2C đang kết nối với cảm biến.
 * @retval Nhiệt độ môi trường đo được.
 */
float BMP280_Read_Temperature(I2C_TypeDef *I2Cx) {
    uint8_t raw_data[3];
    int32_t adc_T;
    int32_t var1, var2, T;

    /* 1. Đọc 3 byte dữ liệu nhiệt độ thô (MSB, LSB, XLSB) */
    BMP280_Read_Bytes(I2Cx, BMP280_REG_TEMP_MSB, raw_data, 3);

    /* 2. Ghép 3 byte thành một biến số nguyên 20-bit (adc_T) */
    adc_T = (int32_t)(((uint32_t)raw_data[0] << 12) |
                      ((uint32_t)raw_data[1] << 4)  |
                      ((uint32_t)raw_data[2] >> 4));

    /* 3. Chạy thuật toán bù nhiệt độ chuẩn của hãng Bosch */
    var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;

    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;

    /* 4. Định dạng đầu ra: Chia cho 100 để trả về độ C dạng số thực */
    return (float)T / 100.0f;
}
