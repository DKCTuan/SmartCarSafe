/**
 * @file    hw_i2c.h
 * @author  Mem 3 - Nguyễn Công Trường
 * @brief   Lớp trừu tượng cho I2C (Hỗ trợ các module I2Cx).
 * Cung cấp các hàm khởi tạo và thao tác truyền/nhận mức thanh ghi.
 */

#ifndef HW_I2C_H
#define HW_I2C_H


#include "stm32f4xx.h"
#include <stdint.h>



/**
 * @brief  Cấu hình một chân GPIO bất kỳ sang chế độ I2C (AF, Open-Drain, Pull-up).
 * @param  GPIOx:  Con trỏ tới Port chứa chân đó (VD: GPIOA, GPIOB).
 * @param  pin:    Số thứ tự chân (0 - 15).
 * @param  af_num: Mã Alternate Function (VD: 4 hoặc 9 cho I2C).
 * @retval None
 */
void HW_GPIO_Init_I2C_Pin(GPIO_TypeDef *GPIOx, uint8_t pin, uint8_t af_num);

/**
 * @brief  Khởi tạo bộ ngoại vi I2C (Cấu hình Baudrate, TRISE, bật I2C).
 * @param  I2Cx: Con trỏ tới ngoại vi I2C cần dùng.
 */
void HW_I2C_Init(I2C_TypeDef *I2Cx);

/**
 * @brief  Phát điều kiện START trên bus I2C.
 * @param  I2Cx: Con trỏ tới ngoại vi I2C đang sử dụng.
 */
void HW_I2C_Start(I2C_TypeDef *I2Cx);

/**
 * @brief  Phát điều kiện STOP trên bus I2C.
 * @param  I2Cx: Con trỏ tới ngoại vi I2C đang sử dụng.
 */
void HW_I2C_Stop(I2C_TypeDef *I2Cx);

/**
 * @brief  Gửi địa chỉ của thiết bị Slave (đã bao gồm bit định hướng R/W).
 * @param  I2Cx:    Con trỏ tới ngoại vi I2C đang sử dụng.
 * @param  address: Địa chỉ 8-bit của thiết bị Slave.
 */
void HW_I2C_WriteAddress(I2C_TypeDef *I2Cx, uint8_t address);

/**
 * @brief  Gửi 1 byte dữ liệu lên bus I2C.
 * @param  I2Cx: Con trỏ tới ngoại vi I2C đang sử dụng.
 * @param  data: Dữ liệu 8 bit cần truyền đi.
 * @retval None
 */
void HW_I2C_WriteData(I2C_TypeDef *I2Cx, uint8_t data);

/**
 * @brief  Đọc 1 byte dữ liệu từ bus I2C và trả về tín hiệu ACK.
 * (Dùng khi muốn báo Slave tiếp tục gửi byte tiếp theo).
 * @param  I2Cx: Con trỏ tới ngoại vi I2C đang sử dụng.
 * @retval uint8_t: Dữ liệu 8-bit đọc được từ thanh ghi nhận.
 */
uint8_t HW_I2C_ReadData_ACK(I2C_TypeDef *I2Cx);

/**
 * @brief  Đọc 1 byte dữ liệu từ bus I2C và trả về tín hiệu NACK kèm STOP.
 * (Dùng khi đọc byte cuối cùng để báo Slave dừng truyền).
 * @param  I2Cx: Con trỏ tới ngoại vi I2C đang sử dụng.
 * @retval uint8_t: Dữ liệu 8-bit đọc được từ thanh ghi nhận.
 */
uint8_t HW_I2C_ReadData_NACK(I2C_TypeDef *I2Cx);

#endif /* HW_I2C_H */
