#include "car_state.h"

/* BIẾN TOÀN CỤC CỤC BỘ (STATIC) PHỤC VỤ THUẬT TOÁN DEBOUNCE */
/* Chỉ số mảng tương ứng với giá trị của Enum Car_Signal_Type_t */
static uint8_t  last_pin_states[CAR_SIGNAL_MAX]   = {1, 1, 1}; /* Mặc định mức Cao (do Pull-up) */
static uint8_t  stable_pin_states[CAR_SIGNAL_MAX] = {1, 1, 1}; /* Trạng thái đã lọc nhiễu */
static uint32_t last_debounce_time[CAR_SIGNAL_MAX] = {0, 0, 0}; /* Mốc thời gian SysTick */

/* ĐỊNH NGHĨA CÁC HÀM CHÍNH                                                */

void Car_State_Init(void) {
    /* BẬT XUNG NHỊP (CLOCK) CHO PORT C */
    /* Sử dụng macro chuẩn của hãng thay vì gán bit (1 << 2) */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

    /* CẤU HÌNH CHẾ ĐỘ ĐẦU VÀO (INPUT MODE) CHO PC0, PC1, PC2 */
    /* Xóa các bit cấu hình cũ để đưa về Mode 00 (Input) */
    CAR_STATE_PORT->MODER &= ~(GPIO_MODER_MODER0 |
                               GPIO_MODER_MODER1 |
                               GPIO_MODER_MODER2);

    /* CẤU HÌNH ĐIỆN TRỞ KÉO LÊN (PULL-UP) CHO PC0, PC1, PC2 */
    /* Bước 1: Xóa trạng thái Pull-up/Pull-down cũ */
    CAR_STATE_PORT->PUPDR &= ~(GPIO_PUPDR_PUPDR0 |
                               GPIO_PUPDR_PUPDR1 |
                               GPIO_PUPDR_PUPDR2);

    /* Bước 2: Ghi giá trị 01 để kích hoạt Pull-up */
    CAR_STATE_PORT->PUPDR |= (GPIO_PUPDR_PUPDR0_0 |
                              GPIO_PUPDR_PUPDR1_0 |
                              GPIO_PUPDR_PUPDR2_0);
}

uint8_t Car_Get_System_Status(Car_Signal_Type_t signal_type) {
    uint32_t pin_mask = 0;
    uint8_t  raw_read = 1;
    uint8_t  index = (uint8_t)signal_type;
    uint8_t  final_status = 0;

    /* KIỂM TRA TÍNH HỢP LỆ CỦA THAM SỐ ĐẦU VÀO */
    if (signal_type >= CAR_SIGNAL_MAX) {
        return 0;
    }

    /* ÁNH XẠ ENUM SANG MACRO MẶT NẠ BIT CỦA TỪNG CHÂN VẬT LÝ */
    switch (signal_type) {
        case CAR_ACC_SIGNAL:
            pin_mask = GPIO_IDR_ID0; /* Mặt nạ bit cho PC0 */
            break;
        case CAR_DOOR_SIGNAL:
            pin_mask = GPIO_IDR_ID1; /* Mặt nạ bit cho PC1 */
            break;
        case CAR_LOCK_SIGNAL:
            pin_mask = GPIO_IDR_ID2; /* Mặt nạ bit cho PC2 */
            break;
        default:
            return 0;
    }

    /* ĐỌC TRẠNG THÁI VẬT LÝ TỨC THỜI TỪ THANH GHI IDR */
    if (CAR_STATE_PORT->IDR & pin_mask) {
        raw_read = 1; /* Nút chưa nhấn (Mức cao do Pull-up) */
    } else {
        raw_read = 0; /* Nút đang nhấn (Bị kéo xuống GND) */
    }

    /* THUẬT TOÁN LỌC NHIỄU (DEBOUNCE) NON-BLOCKING */
    if (raw_read != last_pin_states[index]) {
        /* Nếu có sự thay đổi điện áp, cập nhật lại mốc thời gian đếm */
        last_debounce_time[index] = HAL_GetTick();
    }

    /* Nếu tín hiệu giữ ổn định qua mức thời gian quy định (50ms) */
    if ((HAL_GetTick() - last_debounce_time[index]) > DEBOUNCE_TIME_MS) {
        stable_pin_states[index] = raw_read; /* Chốt trạng thái an toàn */
    }

    /* Lưu lại trạng thái để so sánh cho chu kỳ quét tiếp theo */
    last_pin_states[index] = raw_read;

    /* XỬ LÝ LOGIC CHỨC NĂNG (MẠCH ACTIVE-LOW) */
    /* Nút nhấn xuống GND nên mức 0 tương đương với trạng thái Kích hoạt (Active) */
    switch (signal_type) {
        case CAR_ACC_SIGNAL:
            /* ACC: Nhấn (0) = Bật máy (1). Nhả (1) = Tắt máy (0) */
            final_status = (stable_pin_states[index] == 0) ? 1 : 0;
            break;

        case CAR_DOOR_SIGNAL:
            /* Door: Nhấn (0) = Cửa mở (1). Nhả (1) = Cửa đóng (0) */
            final_status = (stable_pin_states[index] == 0) ? 1 : 0;
            break;

        case CAR_LOCK_SIGNAL:
            /* Lock: Nhấn (0) = Đã chốt khóa (1). Nhả (1) = Chưa khóa (0) */
            final_status = (stable_pin_states[index] == 0) ? 1 : 0;
            break;

        default:
            final_status = 0;
            break;
    }

    return final_status;
}
