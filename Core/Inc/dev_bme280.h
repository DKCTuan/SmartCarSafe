/**
 * @file    dev_bmp280.h
 * @author  Mem 3 - Nguyễn Công Trường
 * @brief   Thư viện điều khiển thiết bị: Cảm biến nhiệt độ/áp suất BMP280.
 */

#ifndef DEV_BMP280_H
#define DEV_BMP280_H

#include <stdint.h>

/**
 * @brief Địa chỉ I2C mặc định của cảm biến BMP280.
 */
#define BMP280_ADDR 0x76


/**
 * @brief  Khởi tạo cảm biến BME280 và đọc các thông số bù trừ.
 * @param  None
 * @retval None
 */
void BMP280_Init(void); //deinit

/**
 * @brief  Đọc và tính toán nhiệt độ thực tế từ cảm biến BMP280.
 * @param  None
 * @retval Giá trị nhiệt độ đo được ở định dạng số thực (Đơn vị: Độ C).
 */
float BMP280_Read_Temperature(void);

#endif /* DEV_BME280_H */
