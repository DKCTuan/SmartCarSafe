/**
 * @file    dev_lcd_i2c.h
 * @author  Mem 3 - Nguyễn Công Trường
 * @brief   Thư viện điều khiển hiển thị: Màn hình LCD 16x2 qua module I2C.
 */

#ifndef DEV_LCD_I2C_H
#define DEV_LCD_I2C_H

#include <stdint.h>

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
 * @param  None
 * @retval None
 */
void LCD_Init(void);

/**
 * @brief  Hiển thị trạng thái hệ thống và nhiệt độ lên màn hình LCD.
 * @param  state       Trạng thái hiện tại của Máy trạng thái (State Machine).
 * (Ví dụ: 0 = Bình thường, 1 = Phát hiện người, 2 = Nguy hiểm)
 * @param  temperature Giá trị nhiệt độ đo được (để in ra màn hình).
 * @retval None
 */
void LCD_Display_Status(uint8_t state, float temperature);

#endif /* DEV_LCD_I2C_H */
