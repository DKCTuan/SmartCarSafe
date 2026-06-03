/*
 * bme280_lcd_i2c.h
 *
 *  Created on: Jun 3, 2026
 *      Author: DINH KIEU CONG TUAN
 */

#ifndef BME280_LCD_I2C_H
#define BME280_LCD_I2C_H

#include "stm32f4xx.h"

/* Hàm khởi tạo I2C và màn hình LCD 16x28 */
void BME280_LCD_Init(void);

/* Hàm đọc và trả về giá trị nhiệt độ từ cảm biến BME280 */
float BME280_Read_Temperature(void);

/* Hàm hiển thị trạng thái hệ thống và nhiệt độ lên màn hình LCD */
void LCD_Display_Status(uint8_t state, float temperature);

#endif /* BME280_LCD_I2C_H */
