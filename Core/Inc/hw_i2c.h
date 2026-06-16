#ifndef HW_I2C_H
#define HW_I2C_H

#include "stm32f4xx.h"
#include <stdint.h>

/* Khởi tạo phần cứng I2C1 (Cấu hình thanh ghi RCC, GPIO, I2C) */
void I2C1_Init(void);

/* Các hàm thao tác truyền/nhận mức thấp */
void I2C1_Start(void);
void I2C1_Stop(void);
void I2C1_WriteAddress(uint8_t address);
void I2C1_WriteData(uint8_t data);
uint8_t I2C1_ReadData_ACK(void);
uint8_t I2C1_ReadData_NACK(void);

#endif /* HW_I2C_H */
