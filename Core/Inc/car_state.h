#ifndef CAR_STATE_H
#define CAR_STATE_H

#include "stm32f4xx.h"
#include <stdint.h>

/**
 * @brief  Định nghĩa thời gian chống dội phím bằng phần mềm (mili-giây)
 */
#define DEBOUNCE_TIME_MS    50

/**
 * @brief  Kiểu dữ liệu enum để quản lý chỉ mục các loại tín hiệu đầu vào của xe
 */
typedef enum {
    CAR_ACC_SIGNAL = 0,
    CAR_DOOR_SIGNAL,
    CAR_LOCK_SIGNAL,
    CAR_SIGNAL_MAX
} Car_Signal_Type_t;

/**
 * @brief  Cấu trúc lưu trữ cấu hình chân GPIO vật lý cho từng tín hiệu
 */
typedef struct {
    GPIO_TypeDef *port;
    uint8_t       pin;
} Car_Pin_Config_t;

/**
 * @brief  Khởi tạo cấu hình thanh ghi GPIO đầu vào cho các tín hiệu của xe
 * @param  config_array: Con trỏ tới mảng chứa cấu hình Port và Pin của các tín hiệu
 */
void Car_State_Init(Car_Pin_Config_t *config_array);

/**
 * @brief  Đọc trạng thái logic an toàn của hệ thống (đã qua thuật toán Debounce và Toggle)
 * @param  signal_type: Loại tín hiệu cần kiểm tra (CAR_ACC_SIGNAL, CAR_DOOR_SIGNAL, CAR_LOCK_SIGNAL)
 * @return Trạng thái logic hiện tại của tín hiệu (0 hoặc 1)
 */
uint8_t Car_Get_System_Status(Car_Signal_Type_t signal_type);

/**
 * @brief  Hủy cấu hình các chân tín hiệu, đưa GPIO về trạng thái analog mode mặc định
 */
void Car_State_DeInit(void);

#endif /* CAR_STATE_H */
