/**
 * @file    dev_lcd_i2c.h
 * @author  Mem 3 - Nguyễn Công Trường
 * @brief   Thư viện điều khiển hiển thị: Màn hình LCD 16x2 qua module I2C.
 */

#ifndef DEV_LCD_I2C_H
#define DEV_LCD_I2C_H

#include <stdint.h>
#include "stm32f4xx.h"

/**
 * @brief Địa chỉ I2C của module chuyển đổi PCF8574 (Gắn sau lưng LCD).
 */

typedef enum {
    SYS_STATE_SAFE = 0,     // Trạng thái an toàn
    SYS_STATE_WARNING,      // Trạng thái cảnh báo
    SYS_STATE_DANGER,       // Trạng thái nguy hiểm
    SYS_STATE_ERROR         // Trạng thái lỗi
} System_State_t;

#define LCD_I2C_ADDR 0x27

/**
 * @brief  Khởi tạo màn hình LCD 16x2.
 * @param  I2Cx: Con trỏ tới ngoại vi I2C (VD: I2C1, I2C2).
 * @retval None
 */
void LCD_Init(I2C_TypeDef *I2Cx);

/**
 * @brief  Hiển thị trạng thái hệ thống và nhiệt độ lên màn hình LCD.
 * @param  I2Cx:        Con trỏ tới ngoại vi I2C.
 * @param  state:       Trạng thái hiện tại.
 * @param  temperature: Giá trị nhiệt độ.
 * @retval None
 */
void LCD_Display_Status(I2C_TypeDef *I2Cx, uint8_t state, float temperature);

#endif /* DEV_LCD_I2C_H */
