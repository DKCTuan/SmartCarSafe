#include "car_state.h"
#include "sys_timer.h"

/* Mảng lưu trữ cấu hình chân được nạp từ hàm init */
static Car_Pin_Config_t signal_pins[CAR_SIGNAL_MAX];

/* Biến phục vụ thuật toán debounce
 * Chỉ số mảng tương ứng với giá trị của enum Car_Signal_Type_t */
static uint8_t  last_pin_states[CAR_SIGNAL_MAX]    = {1, 1, 1}; /* Mặc định mức cao (do pull-up) */
static uint8_t  stable_pin_states[CAR_SIGNAL_MAX]  = {1, 1, 1}; /* Trạng thái đã lọc nhiễu */
static uint32_t last_debounce_time[CAR_SIGNAL_MAX] = {0, 0, 0}; /* Mốc thời gian SysTick */

/* Mảng xử lý tính năng nhấn - nhả */
static uint8_t  last_stable_states[CAR_SIGNAL_MAX]  = {1, 1, 1}; /* Theo dõi trạng thái sườn cũ */
static uint8_t  logic_toggle_states[CAR_SIGNAL_MAX] = {0, 0, 0}; /* Trạng thái giả lập đang lưu trữ (0: Tắt/Đóng, 1: Bật/Mở) */

/*
 * @brief Cấu hình một chân bất kỳ sang input pull-up
 *
 * Bật xung nhịp cho port tương ứng và cấu hình các thanh ghi MODER, PUPDR
 * để đưa chân vật lý về trạng thái đầu vào có điện trở kéo lên.
 */
static void Init_Input_Pullup_Pin(GPIO_TypeDef *GPIOx, uint8_t pin)
{
    /* Bật clock tương ứng cho port */
    switch ((uint32_t)GPIOx)
    {
        case (uint32_t)GPIOA:
            RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
            break;
        case (uint32_t)GPIOB:
            RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
            break;
        case (uint32_t)GPIOC:
            RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
            break;
        default:
            break;
    }

    /* Tính toán vị trí bit bằng phép dịch trái để tối ưu tốc độ xử lý */
    uint8_t bit_pos = pin << 1;

    /* Cấu hình moder: xóa 2 bit về 00 để đưa vào chế độ input */
    GPIOx->MODER &= ~(0b11 << bit_pos);

    /* Cấu hình pupdr: xóa 2 bit cũ và ghi 01 để bật điện trở kéo lên */
    GPIOx->PUPDR &= ~(0b11 << bit_pos);
    GPIOx->PUPDR |=  (0b01 << bit_pos);
}

/*
 * @brief Cấu hình chân bất kỳ về trạng thái analog mode
 *
 * Vô hiệu hóa tính năng input và ngắt điện trở kéo, đưa chân về trạng thái an toàn.
 */
static void DeInit_Input_Pin(GPIO_TypeDef *GPIOx, uint8_t pin)
{
    /* Trở về analog mode (11) */
    GPIOx->MODER |=  (3 << (pin * 2));

    /* Tắt pull-up/pull-down (00) */
    GPIOx->PUPDR &= ~(3 << (pin * 2));
}

/*
 * @brief Khởi tạo các chân tín hiệu đầu vào dựa trên mảng cấu hình phần cứng
 *
 * Hàm quét qua mảng cấu hình truyền từ main.c, tự động cấp xung nhịp
 * và thiết lập chế độ input pull-up cho các chân tương ứng.
 */
void Car_State_Init(Car_Pin_Config_t *config_array)
{
    /* Quét qua toàn bộ mảng cấu hình được truyền vào từ main.c */
    for (uint8_t i = 0; i < CAR_SIGNAL_MAX; i++)
    {
        /* Lưu lại cấu hình vào bộ nhớ nội bộ */
        signal_pins[i].port = config_array[i].port;
        signal_pins[i].pin  = config_array[i].pin;

        /* Gọi hàm gán chân vật lý */
        Init_Input_Pullup_Pin(signal_pins[i].port, signal_pins[i].pin);
    }
}

/*
 * @brief Hủy cấu hình các chân tín hiệu đầu vào
 *
 * Cấu hình chân về trạng thái analog mode mặc định và ngắt điện trở kéo.
 */
void Car_State_DeInit(void)
{
    /* Quét qua toàn bộ mảng chân và gọi hàm deinit động */
    for (uint8_t i = 0; i < CAR_SIGNAL_MAX; i++)
    {
        DeInit_Input_Pin(signal_pins[i].port, signal_pins[i].pin);
    }
}

/*
 * @brief Đọc và trả về trạng thái logic an toàn của hệ thống
 *
 * Đọc tín hiệu vật lý, áp dụng thuật toán chống dội phím (Debounce) non-blocking
 * và bắt sườn âm để lật trạng thái logic (Toggle).
 */
uint8_t Car_Get_System_Status(Car_Signal_Type_t signal_type)
{
    uint8_t raw_read = 1;
    uint8_t index = (uint8_t)signal_type;

    /* Kiểm tra tính hợp lệ của tham số đầu vào */
    if (signal_type >= CAR_SIGNAL_MAX)
    {
        return 0;
    }

    /* Trích xuất port và pin từ mảng lưu trữ */
    GPIO_TypeDef *port = signal_pins[index].port;
    uint8_t       pin  = signal_pins[index].pin;
    uint32_t      pin_mask = (1 << pin);

    /* 1. Đọc trạng thái vật lý tức thời từ thanh ghi IDR */
    if ((port->IDR & pin_mask) != 0)
    {
        raw_read = 1; /* Nút chưa nhấn (mức cao do pull-up) */
    }
    else
    {
        raw_read = 0; /* Nút đang nhấn (bị kéo xuống GND) */
    }

    /* 2. Thuật toán lọc nhiễu (debounce) non-blocking */
    uint32_t current_time = SysTimer_GetTick();

    if (raw_read != last_pin_states[index])
    {
        /* Nếu có sự thay đổi điện áp, cập nhật lại mốc thời gian đếm */
        last_debounce_time[index] = current_time;
    }

    /* Nếu tín hiệu giữ ổn định qua mức thời gian quy định (50ms) */
    if ((current_time - last_debounce_time[index]) > (uint32_t)DEBOUNCE_TIME_MS)
    {
        stable_pin_states[index] = raw_read; /* Chốt trạng thái an toàn */
    }

    /* Lưu lại trạng thái để so sánh cho chu kỳ quét tiếp theo */
    last_pin_states[index] = raw_read;

    /* 3. Thuật toán bắt sườn bằng phần mềm để lật trạng thái (Toggle)
     * Nút là active-low, nên khi stable chuyển từ 1 (nhả) xuống 0 (nhấn) -> có người bấm */
    if ((stable_pin_states[index] == 0) && (last_stable_states[index] == 1))
    {
        /* Đảo ngược trạng thái logic đang lưu trữ (0 thành 1, 1 thành 0) */
        logic_toggle_states[index] = (logic_toggle_states[index] == 0) ? 1 : 0;
    }

    /* Lưu lại trạng thái stable để làm mốc so sánh sườn cho vòng quét sau */
    last_stable_states[index] = stable_pin_states[index];

    /* 4. Trả về trạng thái đã được lật */
    return logic_toggle_states[index];
}
