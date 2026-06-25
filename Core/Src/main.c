/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "car_actuator.h"
#include "car_state.h"
#include "dev_bmp280.h"
#include "dev_lcd_i2c.h"
#include "dev_sound_analog.h"
#include "hw_i2c.h"
#include "radar_exti.h"
#include "sys_timer.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/**
 * @brief Trạng thái máy trạng thái ứng dụng
 * - IDLE: Xe mở khóa hoặc ACC bật, tắt toàn bộ cảnh báo
 * - ARMING: Xe đã khóa, chờ ổn định trước khi quét cabin
 * - SCANNING: Xe đã khóa, giám sát liên tục cảm biến chuyển động/âm thanh/nhiệt độ
 * - ALARM: Phát hiện mối đe dọa, kích hoạt còi và tăng tốc độ quạt
 */
typedef enum
{
    APP_STATE_IDLE = 0U,      /* Trạng thái xe mở khóa */
    APP_STATE_ARMING,         /* Trạng thái chờ ổn định */
    APP_STATE_SCANNING,       /* Trạng thái giám sát chủ động */
    APP_STATE_ALARM           /* Trạng thái phát hiện mối đe dọa */
} App_State_t;

/**
 * @brief Tín hiệu nhập cabin sau khi qua bộ lọc nhiễu/debounce từ các driver
 *
 * Cấu trúc này lưu trữ ảnh chụp tức thời trạng thái ổn định của tất cả tín hiệu nhập.
 * Tầng ứng dụng chỉ xử lý logic qua các trường này, không đọc thanh ghi trực tiếp.
 */
typedef struct
{
    uint8_t acc_on;            /* Trạng thái ACC */
    uint8_t door_open;         /* Trạng thái cửa mở */
    uint8_t door_tamper;       /* Phát hiện cửa bị can thiệp (đã khóa + cửa mở) */
    uint8_t locked;            /* Trạng thái khóa xe */
    uint8_t radar_present;     /* Phát hiện chuyển động bằng radar */
    uint8_t sound_detected;    /* Phát hiện âm thanh vượt ngưỡng */
    float   temperature_c;     /* Nhiệt độ cabin hiện tại tính bằng Celsius */
} Cabin_Input_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define APP_ARMING_TIME_MS          3000U
#define APP_SENSOR_PERIOD_MS        20U
#define APP_TEMP_PERIOD_MS          1000U
#define APP_LCD_PERIOD_MS           2000U
#define APP_ALARM_RECHECK_MS        10000U
#define APP_SOUND_INDICATOR_HOLD_MS 2000U

#define TEMP_THRESHOLD_WARNING_C    30.0f
#define TEMP_THRESHOLD_DANGER_C     33.0f
#define SOUND_THRESHOLD_ADC         500U

/* Giá trị đánh dấu kênh chấp hành không sử dụng */
#define ACTUATOR_UNUSED_PORT        ((GPIO_TypeDef *)0)

/* Chân tín hiệu xe (GPIO input pull-up, active-low) */
#define CAR_ACC_PORT                GPIOC
#define CAR_ACC_PIN                 0U
#define CAR_DOOR_PORT               GPIOC
#define CAR_DOOR_PIN                1U
#define CAR_LOCK_PORT               GPIOC
#define CAR_LOCK_PIN                2U

/* Các chân LED báo trạng thái nút bấm (Port C) */
#define LED_ACC_PORT        		GPIOC
#define LED_ACC_PIN         		3U
#define LED_DOOR_PORT       		GPIOC
#define LED_DOOR_PIN        		4U
#define LED_LOCK_PORT       		GPIOC
#define LED_LOCK_PIN        		5U

/* Cảm biến âm thanh KY-037 */
#define SOUND_PORT                  GPIOA
#define SOUND_PIN                   0U
#define SOUND_ADC_CHANNEL           0U

/* Cảm biến nhiệt độ BMP/BME280 trên I2C1 (PB8/PB9) */
#define BMP_I2C_BUS                 I2C1
#define BMP_I2C_SCL_PORT            GPIOB
#define BMP_I2C_SCL_PIN             8U
#define BMP_I2C_SDA_PORT            GPIOB
#define BMP_I2C_SDA_PIN             9U
#define BMP_I2C_SCL_AF              4U
#define BMP_I2C_SDA_AF              4U

/* Màn hình LCD (qua PCF8574) trên I2C2 (PB10/PB11) */
#define LCD_I2C_BUS                 I2C2
#define LCD_I2C_SCL_PORT            GPIOB
#define LCD_I2C_SCL_PIN             10U
#define LCD_I2C_SDA_PORT            GPIOB
#define LCD_I2C_SDA_PIN             3U
#define LCD_I2C_SCL_AF              4U
#define LCD_I2C_SDA_AF              9U

/* Driver quạt TB6612FNG trên TIM3_CH1 */
#define FAN_AIN1_PORT               GPIOA  /* Chân hướng động cơ 1 (PA7) */
#define FAN_AIN1_PIN                7U
#define FAN_AIN2_PORT               GPIOB  /* Chân hướng động cơ 2 (PB0) */
#define FAN_AIN2_PIN                0U
#define FAN_STBY_PORT               GPIOB  /* Chân standby quạt (PB1) */
#define FAN_STBY_PIN                1U
#define FAN_PWM_PORT                GPIOA  /* Chân PWM quạt (PA6) */
#define FAN_PWM_PIN                 6U
#define SERVO_PWM_PORT              ACTUATOR_UNUSED_PORT
#define SERVO_PWM_PIN               0U
#define TIM3_AF                     2U     /* Hàm thay thế Timer 3 */

/* Các chân đầu ra cảnh báo */
#define BUZZER_PORT                 GPIOA  /* Còi cảnh báo (PA8), mức cao kích hoạt */
#define BUZZER_PIN                  8U
#define SOUND_LED_PORT              GPIOB  /* LED chỉ báo phát hiện âm thanh (PB4) */
#define SOUND_LED_PIN               4U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* Biến lưu trữ trạng thái máy trạng thái hiện tại */
static App_State_t   sg_app_state = APP_STATE_IDLE;

/* Biến lưu trữ ảnh chụp tín hiệu nhập cabin sau lọc */
static Cabin_Input_t sg_input = {0U, 0U, 0U, 0U, 0U, 0U, 28.0f};

/* Thời điểm chuyển đổi trạng thái (ms) */
static uint32_t sg_state_tick = 0U;

/* Thời điểm đọc cảm biến nhanh cuối cùng (ms) */
static uint32_t sg_last_sensor_tick = 0U;

/* Thời điểm đọc nhiệt độ cuối cùng (ms) */
static uint32_t sg_last_temp_tick = 0U;

/* Thời điểm cập nhật màn hình LCD cuối cùng (ms) */
static uint32_t sg_last_lcd_tick = 0U;

/* Thời điểm phát hiện âm thanh cuối cùng (ms) */
static uint32_t sg_last_sound_tick = 0U;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
/*
 * @brief Khởi tạo ứng dụng: cấu hình tất cả các thiết bị ngoại vi và driver
 */
static void APP_Init(void);

/*
 * @brief Vòng lặp chính ứng dụng: xử lý cảm biến và cập nhật máy trạng thái
 */
static void APP_Process(void);

/*
 * @brief Đọc tín hiệu nhập nhanh: trạng thái nút bấm, radar, phát hiện âm thanh (chu kỳ 20ms)
 */
static void APP_ReadFastInputs(void);

/*
 * @brief Đọc tín hiệu nhập chậm: cảm biến nhiệt độ qua I2C (chu kỳ 1s)
 */
static void APP_ReadSlowInputs(void);

/*
 * @brief Cập nhật máy trạng thái và xử lý chuyển đổi trạng thái
 */
static void APP_RunStateMachine(void);

/*
 * @brief Cập nhật các thiết bị đầu ra dựa trên trạng thái hiện tại
 */
static void APP_UpdateOutputs(void);

/*
 * @brief Cập nhật màn hình LCD với trạng thái hệ thống và nhiệt độ hiện tại
 */
static void APP_UpdateDisplay(void);

/*
 * @brief Chống dội và khóa tín hiệu chuyển đổi khóa
 * @param door_open Trạng thái cửa mở hiện tại
 * @return Trạng thái khóa khóa (1=khóa, 0=mở khóa)
 */
static uint8_t APP_ReadLockToggle(uint8_t door_open);

/*
 * @brief Kiểm tra xem các điều kiện bảo vệ có được đáp ứng hay không (ACC tắt, cửa đóng, đã khóa)
 * @return 1 nếu có thể bảo vệ, 0 nếu không
 */
static uint8_t APP_IsArmedCondition(void);

/*
 * @brief Kiểm tra xem các điều kiện hủy bảo vệ có được đáp ứng hay không (ACC bật hoặc xe mở khóa)
 * @return 1 nếu nên hủy bảo vệ, 0 nếu không
 */
static uint8_t APP_IsDisarmCondition(void);

/*
 * @brief Kiểm tra xem có bất kỳ điều kiện báo động nào được kích hoạt hay không
 * @return 1 nếu nên báo động, 0 nếu không
 */
static uint8_t APP_ShouldAlarm(void);

/*
 * @brief Lấy trạng thái hiển thị LCD dựa trên điều kiện hiện tại
 * @return Mã trạng thái hệ thống (AN_TOÀN/CẢNH_BÁO/NGUY_HIỂM)
 */
static uint8_t APP_LcdState(void);

/*
 * @brief Kiểm tra xem LED chỉ báo âm thanh có nên hoạt động hay không
 * @return 1 nếu hoạt động, 0 nếu không hoạt động
 */
static uint8_t APP_IsSoundIndicatorActive(void);

/*
 * @brief Khởi tạo chân đầu ra còi (PA8)
 */
static void APP_Buzzer_Init(void);

/*
 * @brief Điều khiển đầu ra còi
 * @param on 1 để bật còi, 0 để tắt
 */
static void APP_Buzzer_Write(uint8_t on);

/*
 * @brief Khởi tạo chân LED chỉ báo âm thanh (PB4)
 */
static void APP_SoundLed_Init(void);

/*
 * @brief Điều khiển LED chỉ báo âm thanh
 * @param on 1 để bật LED, 0 để tắt
 */
static void APP_SoundLed_Write(uint8_t on);

/*
 * @brief Điều khiển LED trạng thái chính (LD2)
 * @param on 1 để bật LED, 0 để tắt
 */
static void APP_Led_Write(uint8_t on);

/*
 * @brief Nhấp nháy LED với chu kỳ xác định (cho các mẫu nhấp nháy)
 * @param period_ms Chu kỳ nhấp nháy tính bằng mili giây
 */
static void APP_Led_TogglePattern(uint32_t period_ms);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/*
 * @brief Khởi tạo ứng dụng: cấu hình tất cả các thiết bị ngoại vi và driver
 *
 * Thiết lập bộ điều khiển hệ thống, các driver GPIO cho tín hiệu xe, các bus I2C cho cảm biến,
 * ADC cho phát hiện âm thanh, và các thiết bị đầu ra (quạt, còi, LED).
 */
static void APP_Init(void)
{
    /* Cấu hình các chân tín hiệu xe */
    Car_Pin_Config_t car_pins[CAR_SIGNAL_MAX] = {
        {CAR_ACC_PORT,  CAR_ACC_PIN},
        {CAR_DOOR_PORT, CAR_DOOR_PIN},
        {CAR_LOCK_PORT, CAR_LOCK_PIN}
    };

    /* Cấu hình cảm biến âm thanh (KY-037 -> ADC1 kênh 0) */
    Sound_Config_t sound_config = {
        .port = SOUND_PORT,
        .pin = SOUND_PIN,
        .adcChannel = SOUND_ADC_CHANNEL,
        .adc = ADC1
    };

    /* Cấu hình driver quạt (TB6612FNG) đều khiển quạt */
    Car_Actuator_Config_t actuator_config = {
        .ain1 = {FAN_AIN1_PORT, FAN_AIN1_PIN},
        .ain2 = {FAN_AIN2_PORT, FAN_AIN2_PIN},
        .stby = {FAN_STBY_PORT, FAN_STBY_PIN},
        .pwma = {FAN_PWM_PORT, FAN_PWM_PIN, TIM3_AF},
        .servo = {SERVO_PWM_PORT, SERVO_PWM_PIN, TIM3_AF},
        .led_acc  = {LED_ACC_PORT, LED_ACC_PIN},
        .led_door = {LED_DOOR_PORT, LED_DOOR_PIN},
        .led_lock = {LED_LOCK_PORT, LED_LOCK_PIN}
    };

    /* Khởi tạo bộ đều khiển thời gian hệ thống và các thiết bị đầu ra */
    SysTimer_Init(SystemCoreClock);
    APP_Buzzer_Init();
    APP_SoundLed_Init();

    /* Khởi tạo giám sát tín hiệu xe */
    Car_State_Init(car_pins);
    Car_Actuator_Init(&actuator_config);

    /* Khởi tạo I2C1 cho cảm biến nhiệt độ BMP/BME280 */
    HW_GPIO_Init_I2C_Pin(BMP_I2C_SCL_PORT, BMP_I2C_SCL_PIN, BMP_I2C_SCL_AF);
    HW_GPIO_Init_I2C_Pin(BMP_I2C_SDA_PORT, BMP_I2C_SDA_PIN, BMP_I2C_SDA_AF);
    HW_I2C_Init(BMP_I2C_BUS);
    BMP280_Init(BMP_I2C_BUS);

    /* Khởi tạo I2C2 cho màn hình LCD (qua PCF8574 I2C expander) */
    HW_GPIO_Init_I2C_Pin(LCD_I2C_SCL_PORT, LCD_I2C_SCL_PIN, LCD_I2C_SCL_AF);
    HW_GPIO_Init_I2C_Pin(LCD_I2C_SDA_PORT, LCD_I2C_SDA_PIN, LCD_I2C_SDA_AF);
    HW_I2C_Init(LCD_I2C_BUS);
    LCD_Init(LCD_I2C_BUS);

    /* Khởi tạo radar và phát hiện âm thanh */
    Radar_EXTI_Init();
    Sound_Init(&sound_config);
    Sound_SetThreshold(SOUND_THRESHOLD_ADC);

    /* Đặt tất cả các đầu ra về trạng thái an toàn */
    Actuator_Set_Fan_Speed(0U);
    APP_Buzzer_Write(0U);
    APP_Led_Write(0U);
    APP_SoundLed_Write(0U);

    sg_state_tick = SysTimer_GetTick();
}

static void APP_Process(void)
{
    uint32_t now = SysTimer_GetTick();

    /* Liên tục xử lý ADC âm thanh lọc trung bình động */
    Sound_Process();

    /* Vòng lặp cảm biến nhanh: xử lý nút bấm, radar, âm thanh và máy trạng thái (chu kỳ 20ms) */
    if ((now - sg_last_sensor_tick) >= APP_SENSOR_PERIOD_MS)
    {
        sg_last_sensor_tick = now;
        APP_ReadFastInputs();
        APP_RunStateMachine();
        APP_UpdateOutputs();
    }

    /* Vòng lặp cảm biến chậm: đọc nhiệt độ từ I2C (chu kỳ 1s để tránh làm chậm vòng đọc chính) */
    if ((now - sg_last_temp_tick) >= APP_TEMP_PERIOD_MS)
    {
        sg_last_temp_tick = now;
        APP_ReadSlowInputs();
    }

    /* Vòng lặp cập nhật màn hình: làm mới màn hình LCD (chu kỳ 2s để giảm nhấp nháy) */
    if ((now - sg_last_lcd_tick) >= APP_LCD_PERIOD_MS)
    {
        sg_last_lcd_tick = now;
        APP_UpdateDisplay();
    }
}

/*
 * @brief Đọc tín hiệu nhập nhanh: nút bấm, radar, và phát hiện âm thanh
 *
 * Cập nhật cấu trúc sg_input với các trạng thái cảm biến lọc được hiện tại.
 * Phát hiện cửa bị can thiệp kểy ra khi xe đã khóa nhưng cửa mở.
 */
static void APP_ReadFastInputs(void)
{
    /* Đọc tín hiệu xe từ các driver (vãn được chống dội) */
    uint8_t door_requested = Car_Get_System_Status(CAR_DOOR_SIGNAL);

    sg_input.acc_on = Car_Get_System_Status(CAR_ACC_SIGNAL);
    sg_input.locked = APP_ReadLockToggle(door_requested);
    sg_input.door_tamper = ((sg_input.locked != 0U) && (door_requested != 0U)) ? 1U : 0U;
    sg_input.door_open = (sg_input.locked == 0U) ? door_requested : 0U;
    sg_input.radar_present = Radar_Is_Detected();
    sg_input.sound_detected = Sound_IsDetected();

    /* Cập nhật dấu thời gian chỉ báo âm thanh khi phát hiện âm thanh trong quá trình giám sát */
    if (((sg_app_state == APP_STATE_SCANNING) ||
         (sg_app_state == APP_STATE_ALARM)) &&
        (sg_input.sound_detected != 0U))
    {
        sg_last_sound_tick = SysTimer_GetTick();
    }

    /* Cập nhật LED */
    Actuator_Set_StatusLED(sg_input.acc_on, door_requested, sg_input.locked);
}

/*
 * @brief Đọc tín hiệu nhập chậm: cảm biến nhiệt độ qua I2C
 */
static void APP_ReadSlowInputs(void)
{
    sg_input.temperature_c = BMP280_Read_Temperature(BMP_I2C_BUS);
}

/*
 * @brief Cập nhật máy trạng thái và xử lý chuyển đổi
 *
 * Luồng trạng thái: IDLE -> ARMING -> SCANNING -> ALARM
 * Quay trở về IDLE nếu ACC bật hoặc xe mở khóa
 */
static void APP_RunStateMachine(void)
{
    uint32_t now = SysTimer_GetTick();

    switch (sg_app_state)
    {
        case APP_STATE_IDLE:
            if (APP_IsArmedCondition() != 0U)
            {
                sg_app_state = APP_STATE_ARMING;
                sg_state_tick = now;
                Radar_ClearPresence();
            }
            break;

        case APP_STATE_ARMING:
            if (APP_IsDisarmCondition() != 0U)
            {
                sg_app_state = APP_STATE_IDLE;
                sg_state_tick = now;
            }
            else if (sg_input.door_tamper != 0U)
            {
                sg_app_state = APP_STATE_ALARM;
                sg_state_tick = now;
            }
            else if ((now - sg_state_tick) >= APP_ARMING_TIME_MS)
            {
                sg_app_state = APP_STATE_SCANNING;
                sg_state_tick = now;
            }
            break;

        case APP_STATE_SCANNING:
            if (APP_IsDisarmCondition() != 0U)
            {
                sg_app_state = APP_STATE_IDLE;
                sg_state_tick = now;
                Radar_ClearPresence();
            }
            else if (APP_ShouldAlarm() != 0U)
            {
                sg_app_state = APP_STATE_ALARM;
                sg_state_tick = now;
            }
            break;

        case APP_STATE_ALARM:
            if (APP_IsDisarmCondition() != 0U)
            {
                sg_app_state = APP_STATE_IDLE;
                sg_state_tick = now;
                Radar_ClearPresence();
            }
            else
            {
                /* Xóa latch radar để ngăn ngừa dương tính khóa trong khi còi đang kêu */
                /* Tiếng ồn/giữ có thể gây khóa lưu dãy không mong muốn */
                Radar_ClearPresence();
                sg_input.radar_present = RADAR_ABSENCE;

                if ((now - sg_state_tick) >= APP_ALARM_RECHECK_MS)
                {
                    sg_state_tick = now;
                    if (APP_ShouldAlarm() == 0U)
                    {
                        sg_app_state = APP_STATE_SCANNING;
                    }
                }
            }
            break;

        default:
            sg_app_state = APP_STATE_IDLE;
            sg_state_tick = now;
            break;
    }
}

static void APP_UpdateOutputs(void)
{
    switch (sg_app_state)
    {
        case APP_STATE_IDLE:
            Actuator_Set_Fan_Speed(0U);
            APP_Buzzer_Write(0U);
            APP_Led_Write(0U);
            break;

        case APP_STATE_ARMING:
            Actuator_Set_Fan_Speed(0U);
            APP_Buzzer_Write(0U);
            APP_Led_TogglePattern(250U);
            break;

        case APP_STATE_SCANNING:
            APP_Buzzer_Write(0U);
            APP_Led_TogglePattern(1000U);

            if (sg_input.temperature_c >= TEMP_THRESHOLD_WARNING_C)
            {
                Actuator_Set_Fan_Speed(1U);
            }
            else
            {
                Actuator_Set_Fan_Speed(0U);
            }
            break;

        case APP_STATE_ALARM:
            Actuator_Set_Fan_Speed(2U);
            APP_Buzzer_Write(1U);
            APP_Led_Write(1U);
            break;

        default:
            break;
    }

    if ((sg_app_state == APP_STATE_SCANNING) ||
        (sg_app_state == APP_STATE_ALARM))
    {
        APP_SoundLed_Write(APP_IsSoundIndicatorActive());
    }
    else
    {
        APP_SoundLed_Write(0U);
    }
}

/*
 * @brief Cập nhật màn hình LCD với trạng thái hệ thống và nhiệt độ hiện tại
 */
static void APP_UpdateDisplay(void)
{
    LCD_Display_Status(LCD_I2C_BUS, APP_LcdState(), sg_input.temperature_c);
}

/*
 * @brief Chống dội và khóa tín hiệu chuyển đổi khóa từ nguồn vật lý
 *
 * Cùng mọc logic chống dội: đổi trạng thái chỉ khi ổn định cho
 * DEBOUNCE_TIME_MS. Ngăn ngġa đổi trạng thái đôi dựa do nhiễuù.
 * @param door_open Trạng thái cửa mở hiện tại
 * @return Trạng thái khóa khóa (1=khóa, 0=mở khóa)
 */
static uint8_t APP_ReadLockToggle(uint8_t door_open)
{
    static uint8_t  last_raw_state = 1U;
    static uint8_t  stable_state = 1U;
    static uint8_t  last_stable_state = 1U;
    static uint8_t  lock_latched = 0U;
    static uint32_t last_debounce_tick = 0U;

    uint32_t now = SysTimer_GetTick();
    uint32_t pin_mask = (1UL << CAR_LOCK_PIN);
    uint8_t raw_state = ((CAR_LOCK_PORT->IDR & pin_mask) != 0U) ? 1U : 0U;

    if (raw_state != last_raw_state)
    {
        last_debounce_tick = now;
    }

    if ((now - last_debounce_tick) > DEBOUNCE_TIME_MS)
    {
        stable_state = raw_state;
    }

    if ((stable_state == 0U) && (last_stable_state == 1U))
    {
        if (lock_latched != 0U)
        {
            lock_latched = 0U;
        }
        else if (door_open == 0U)
        {
            lock_latched = 1U;
        }
    }

    last_raw_state = raw_state;
    last_stable_state = stable_state;

    return lock_latched;
}

/*
 * @brief Kiểm tra xem xe có thể được bảo vệ hay không (ACC tắt, cửa đóng, khóa)
 * @return 1 nếu điều kiện bảo vệ được đáp ứng, 0 nếu không
 */
static uint8_t APP_IsArmedCondition(void)
{
    return ((sg_input.acc_on == 0U) &&
            (sg_input.door_open == 0U) &&
            (sg_input.locked != 0U)) ? 1U : 0U;
}

/*
 * @brief Kiểm tra xem xe có nên hủy bảo vệ hay không (ACC bật hoặc xe mở khóa)
 * @return 1 nếu điều kiện hủy bảo vệ được đáp ứng, 0 nếu không
 */
static uint8_t APP_IsDisarmCondition(void)
{
    return ((sg_input.acc_on != 0U) ||
            (sg_input.locked == 0U)) ? 1U : 0U;
}

/*
 * @brief Kiểm tra xem có bất kỳ điều kiện báo động nào được kích hoạt hay không
 *
 * Kích hoạt báo động khi:
 * - Cửa bị can thiệp (đã khóa + cửa mở)
 * - Nhiệt độ nguy hiểm (>= 33°C)
 * - Phát hiện âm thanh
 * - Chuyển động + nhiệt độ cảnh báo (radar + >= 30°C)
 *
 * @return 1 nếu có điều kiện báo động, 0 nếu không
 */
static uint8_t APP_ShouldAlarm(void)
{
    /* Báo động ngay: cửa bị can thiệp trong khi đã khóa */
    if (sg_input.door_tamper != 0U)
    {
        return 1U;
    }

    /* Báo động ngay: nhiệt độ nguy hiểm */
    if (sg_input.temperature_c >= TEMP_THRESHOLD_DANGER_C)
    {
        return 1U;
    }

    /* Báo động ngay: phát hiện âm thanh vượt ngưỡng */
    if (sg_input.sound_detected != 0U)
    {
        return 1U;
    }

    /* Báo động kết hợp: chuyển động + nhiệt độ cảnh báo (giảm khả năng dương tính giả) */
    if ((sg_input.radar_present == RADAR_PRESENCE) &&
        (sg_input.temperature_c >= TEMP_THRESHOLD_WARNING_C))
    {
        return 1U;
    }

    return 0U;
}

/*
 * @brief Lấy trạng thái hiển thị LCD dựa trên điều kiện hiện tại
 * @return Mã trạng thái hệ thống (NGUY_HIỄM/CẢNH_BÁO/AN_TOÀN)
 */
static uint8_t APP_LcdState(void)
{
    /* Hiển thị NGUY_HIỄM nếu báo động đang hoạt động */
    if (sg_app_state == APP_STATE_ALARM)
    {
        return SYS_STATE_DANGER;
    }

    /* Hiển thị CẢNH_BÁO trong chu kỳ chờ ổn định */
    if (sg_app_state == APP_STATE_ARMING)
    {
        return SYS_STATE_WARNING;
    }

    /* Trong chế độ giám sát, hiển thị CẢNH_BÁO nếu có mối đe phát hiện */
    if (sg_app_state == APP_STATE_SCANNING)
    {
        if ((sg_input.temperature_c >= TEMP_THRESHOLD_WARNING_C) ||
            (sg_input.radar_present == RADAR_PRESENCE) ||
            (sg_input.sound_detected != 0U))
        {
            return SYS_STATE_WARNING;
        }
    }

    return SYS_STATE_SAFE;
}

/*
 * @brief Kiểm tra xem LED chỉ báo âm thanh có nên hoạt động hay không
 * @return 1 nếu hoạt động (phát hiện âm thanh gần đây), 0 nếu không hoạt động
 */
static uint8_t APP_IsSoundIndicatorActive(void)
{
    if (sg_last_sound_tick == 0U)
    {
        return 0U;
    }

    /* LED lưu sáng trong APP_SOUND_INDICATOR_HOLD_MS sau lần phát hiện cuối cùng */
    return ((SysTimer_GetTick() - sg_last_sound_tick) < APP_SOUND_INDICATOR_HOLD_MS) ? 1U : 0U;
}

/*
 * @brief Khởi tạo chân đầu ra còi (PA8)
 * Cấu hình thành push-pull output không có pull resistor
 */
static void APP_Buzzer_Init(void)
{
    /* Bật mộ đồng hồ GPIO A */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    
    /* Cấu hình PA8: chế độ đầu ra push-pull */
    BUZZER_PORT->MODER &= ~(3U << (BUZZER_PIN * 2U));
    BUZZER_PORT->MODER |=  (1U << (BUZZER_PIN * 2U));
    BUZZER_PORT->OTYPER &= ~(1U << BUZZER_PIN);
    BUZZER_PORT->PUPDR &= ~(3U << (BUZZER_PIN * 2U));
    BUZZER_PORT->OSPEEDR &= ~(3U << (BUZZER_PIN * 2U));
    BUZZER_PORT->ODR &= ~(1U << BUZZER_PIN);  /* Khởi tạo tắt */
}

/*
 * @brief Điều khiển đầu ra còi (hoạt động mức cao)
 * @param on 1 để bật còi, 0 để tắt
 */
static void APP_Buzzer_Write(uint8_t on)
{
    if (on != 0U)
    {
        BUZZER_PORT->ODR |= (1U << BUZZER_PIN);
    }
    else
    {
        BUZZER_PORT->ODR &= ~(1U << BUZZER_PIN);
    }
}

/*
 * @brief Khởi tạo chân LED chỉ báo âm thanh (PB4)
 * Cấu hình thành push-pull output không có pull resistor
 */
static void APP_SoundLed_Init(void)
{
    /* Bật mộ đồng hồ GPIO B */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    
    /* Cấu hình PB4: chế độ đầu ra push-pull */
    SOUND_LED_PORT->MODER &= ~(3U << (SOUND_LED_PIN * 2U));
    SOUND_LED_PORT->MODER |=  (1U << (SOUND_LED_PIN * 2U));
    SOUND_LED_PORT->OTYPER &= ~(1U << SOUND_LED_PIN);
    SOUND_LED_PORT->PUPDR &= ~(3U << (SOUND_LED_PIN * 2U));
    SOUND_LED_PORT->OSPEEDR &= ~(3U << (SOUND_LED_PIN * 2U));
    SOUND_LED_PORT->ODR &= ~(1U << SOUND_LED_PIN);  /* Khởi tạo tắt */
}

/*
 * @brief Điều khiển LED chỉ báo âm thanh
 * @param on 1 để bật LED, 0 để tắt
 */
static void APP_SoundLed_Write(uint8_t on)
{
    if (on != 0U)
    {
        SOUND_LED_PORT->ODR |= (1U << SOUND_LED_PIN);
    }
    else
    {
        SOUND_LED_PORT->ODR &= ~(1U << SOUND_LED_PIN);
    }
}

/*
 * @brief Điều khiển LED trạng thái chính (LD2)
 * @param on 1 để bật LED, 0 để tắt
 */
static void APP_Led_Write(uint8_t on)
{
    if (on != 0U)
    {
        LD2_GPIO_Port->ODR |= LD2_Pin;
    }
    else
    {
        LD2_GPIO_Port->ODR &= ~LD2_Pin;
    }
}

/*
 * @brief Nhấp nháy LED với chu kỳ xác định (cho các mẫu nhấp nháy)
 * @param period_ms Chu kỳ nhấp nháy tính bằng mili giây
 */
static void APP_Led_TogglePattern(uint32_t period_ms)
{
    uint32_t phase = (SysTimer_GetTick() / period_ms) & 1U;
    APP_Led_Write((uint8_t)(phase != 0U));
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* GHI CHÙ: SystemClock_Config() được vô hiệu hóa để sử dụng đồng hồ HSI 16MHz mặc định
   * Điều này phù hợp với cấu hình bus I2C trong hw_i2c.c (APB1 = 16MHz)
   */
  /* SystemClock_Config(); */

  /* USER CODE END Init */

  /* Khởi tạo tất cả các thiết bị ngoại vi được cấu hình */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  APP_Init();  /* Khởi tạo các driver và thiết bị tầng ứng dụng */
  /* USER CODE END 2 */

  /* Vòng lặp chính ứng dụng */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      APP_Process();  /* Xử lý cảm biến và cập nhật máy trạng thái */
  }
  /* USER CODE END 3 */
}
/*
 * @brief Cấu hình Đồng hồ Hệ thống (hiện tại được vô hiệu hóa)
 *
 * Vô hiệu hóa để sử dụng đồng hồ HSI 16MHz mặc định.
 * Driver I2C hw_i2c.c yêu cầu APB1 = 16MHz để cấu hình thời gian.
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
