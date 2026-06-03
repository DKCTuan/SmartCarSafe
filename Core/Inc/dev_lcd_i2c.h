#ifndef DEV_LCD_I2C_H
#define DEV_LCD_I2C_H

#include <stdint.h>

/* Địa chỉ module I2C PCF8574 của LCD */
#define LCD_I2C_ADDR 0x27

/* Hàm khởi tạo màn hình LCD 16x2 */
void LCD_Init(void);

/* Hàm hiển thị trạng thái hệ thống và nhiệt độ lên màn hình LCD */
void LCD_Display_Status(uint8_t state, float temperature);

#endif /* DEV_LCD_I2C_H */
