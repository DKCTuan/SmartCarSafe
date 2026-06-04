/**
 * @file    hw_i2c.h
 * @author  Mem 3 - Nguyễn Công Trường
 * @brief   Lớp trừu tượng cho giao tiếp I2C1.
 */

#ifndef HW_I2C_H
#define HW_I2C_H


#include "stm32f4xx.h"
#include <stdint.h>


/**
 * @brief  Khởi tạo bộ ngoại vi I2C1 phần cứng.
 * @param  None
 * @retval None
 */
void I2C1_Init(void);

/**
 * @brief  Phát điều kiện START trên bus I2C.
 * @param  None
 * @retval None
 */
void I2C1_Start(void);

/**
 * @brief  Phát điều kiện STOP trên bus I2C.
 * @param  None
 * @retval None
 */
void I2C1_Stop(void);

/**
 * @brief  Gửi địa chỉ của thiết bị Slave kèm bit định hướng .
 * @param  address Địa chỉ của thiết bị Slave .
 * @retval None
 */
void I2C1_WriteAddress(uint8_t address);

/**
 * @brief  Gửi 1 byte dữ liệu lên bus I2C.
 * @param  data Giá trị (8-bit) cần gửi.
 * @retval None
 */
void I2C1_WriteData(uint8_t data);

/**
 * @brief  Đọc 1 byte dữ liệu từ bus I2C và trả về tín hiệu ACK.
 * @param  None
 * @retval Dữ liệu (8-bit) đọc được từ thanh ghi nhận (DR).
 */
uint8_t I2C1_ReadData_ACK(void);

/**
 * @brief  Đọc 1 byte dữ liệu từ bus I2C và trả về tín hiệu NACK.
 * @param  None
 * @retval Dữ liệu (8-bit) đọc được từ thanh ghi nhận (DR).
 */
uint8_t I2C1_ReadData_NACK(void);

#endif /* HW_I2C_H */
