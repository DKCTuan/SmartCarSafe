#ifndef CAR_STATE_H
#define CAR_STATE_H

#include "stm32f4xx.h"
#include <stdint.h>

/* --- ĐỊNH NGHĨA CHÂN ĐẦU VÀO (CÔNG TẮC)
 * Dùng Port B cho các công tắc */
#define INPUT_PORT          GPIOB
/* PB0 - Nút mô phỏng ACC */
#define ACC_PIN             0   // PB0 - Nút mô phỏng ACC
#define DOOR_PIN            1   // PB1 - Nút mô phỏng Cửa xe
#define LOCK_PIN            2   // PB2 - Nút mô phỏng Khóa cửa

/* --- CẤU HÌNH THỜI GIAN DEBOUNCE --- */
#define DEBOUNCE_DELAY_MS   50  // 50 mili-giây

/* Khởi tạo phần cứng vi điều khiển để sẵn sàng đọc tín hiệu từ các công tắc. */
void Car_State_Init(void);

/* Kiểm tra xem khóa điện của xe (ACC) đang bật hay tắt. */
uint8_t Car_Get_ACC_Status(void);

/* Giám sát xem có cánh cửa xe nào đang mở vật lý hay không */
uint8_t Car_Get_Door_Status(void);

/* Kiểm tra xem người dùng đã bấm chốt khóa cửa (Lock) hay chưa. */
uint8_t Car_Get_Lock_Status(void);

#endif /* CAR_STATE_H */
