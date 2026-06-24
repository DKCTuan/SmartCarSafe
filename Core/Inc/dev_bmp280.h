/**
 * @file    dev_bmp280.h
 * @author  Mem 3 - Nguyễn Công Trường
 * @brief   Header file cho cảm biến BMP280 (Hỗ trợ đa module I2Cx)
 */

#ifndef DEV_BMP280_H
#define DEV_BMP280_H

#include "stm32f4xx.h"
#include <stdint.h>

/* Đặt địa chỉ module BMP280 là 0x76 bằng cách nối SDO với GND */
#define BMP280_ADDR 0x76


/**
 * @brief  Khởi tạo cảm biến BMP280, đọc ID, lấy hằng số bù nhiệt và cấu hình
 * @param  I2Cx: Con trỏ tới ngoại vi I2C đang kết nối với cảm biến (VD: I2C1, I2C2)
 */
void BMP280_Init(I2C_TypeDef *I2Cx);

/**
 * @brief  Hủy khởi tạo cảm biến BMP280 (Đưa về Sleep mode hoặc Soft Reset)
 * @param  I2Cx: Con trỏ tới ngoại vi I2C đang kết nối với cảm biến
 */
void BMP280_DeInit(I2C_TypeDef *I2Cx);

/**
 * @brief  Đọc dữ liệu thô và tính toán ra nhiệt độ thực tế
 * @param  I2Cx: Con trỏ tới ngoại vi I2C đang kết nối với cảm biến
 * @retval Nhiệt độ đã tính toán.
 */
float BMP280_Read_Temperature(I2C_TypeDef *I2Cx);

#endif /* DEV_BMP280_H */
