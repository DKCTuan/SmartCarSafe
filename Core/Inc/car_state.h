#ifndef CAR_STATE_H
#define CAR_STATE_H

#include "stm32f4xx.h"
#include <stdint.h>

/* Định nghĩa thời gian chống dội phím (mili-giây) */
#define DEBOUNCE_TIME_MS    50

/* Định nghĩa kiểu dữ liệu enum để quản lý các loại tín hiệu đầu vào của xe */
typedef enum {
    CAR_ACC_SIGNAL = 0,
    CAR_DOOR_SIGNAL,
    CAR_LOCK_SIGNAL,
    CAR_SIGNAL_MAX
} Car_Signal_Type_t;

/* Định nghĩa kiểu dữ liệu lưu trữ cấu hình chân */
typedef struct {
    GPIO_TypeDef *port;
    uint8_t       pin;
} Car_Pin_Config_t;

/* Hàm khởi tạo cấu hình thanh ghi GPIO input */
void Car_State_Init(Car_Pin_Config_t *config_array);

/* Hàm đọc trạng thái xe (có debounce) */
uint8_t Car_Get_System_Status(Car_Signal_Type_t signal_type);

/* Hàm hủy cấu hình chân */
void Car_State_DeInit(void);

#endif /* CAR_STATE_H */
