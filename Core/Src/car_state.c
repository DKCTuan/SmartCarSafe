/*
 * car_switch.c
 *
 *
 */

#include "car_actuator.h"
#include <stdint.h>

/* Khởi tạo cấu hình các chân GPIO đầu vào cho ACC, Cửa xe và Khóa cửa */
void Car_State_Init(void);

/* Đọc trạng thái khóa điện ACC (trả về 1 nếu ON, 0 nếu OFF - có xử lý debounce) */
uint8_t Car_Get_ACC_Status(void);

/* Đọc trạng thái vật lý của cửa xe (trả về 1 nếu Mở, 0 nếu Đóng - có xử lý debounce) */
uint8_t Car_Get_Door_Status(void);

/* Đọc trạng thái chốt khóa cửa (trả về 1 nếu Đã khóa, 0 nếu Chưa khóa - có xử lý debounce) */
uint8_t Car_Get_Lock_Status(void);
