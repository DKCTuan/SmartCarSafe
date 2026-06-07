#ifndef CAR_STATE_H
#define CAR_STATE_H

#include "stm32f4xx.h"
#include <stdint.h>

/* Định nghĩa các hằng số về chân cắm */
#define CAR_STATE_PORT      GPIOC

/* Định nghĩa thời gian chống dội phím (mili-giây) */
#define DEBOUNCE_TIME_MS    50

/* Định nghĩa kiểu dữ liệu Enum để quản lý các loại tín hiệu đầu vào của xe */
typedef enum {
    CAR_ACC_SIGNAL = 0,
    CAR_DOOR_SIGNAL,
    CAR_LOCK_SIGNAL,
    CAR_SIGNAL_MAX      /* Dùng để giới hạn số lượng phần tử mảng */
} Car_Signal_Type_t;

/* Hàm khởi tạo cấu hình thanh ghi GPIO Input cho cả 3 chân PC0, PC1, PC2 */
void Car_State_Init(void);

/* Hàm đọc trạng thái xe có Debounce */
uint8_t Car_Get_System_Status(Car_Signal_Type_t signal_type);

#endif /* CAR_STATE_H */
