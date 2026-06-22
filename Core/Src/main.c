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
typedef enum
{
    /* IDLE: xe chua khoa hoac dang co nguoi/ACC, tat toan bo canh bao */
    APP_STATE_IDLE = 0U,

    /* ARMING: da khoa xe, doi on dinh trong vai giay truoc khi quet cabin */
    APP_STATE_ARMING,

    /* SCANNING: xe da khoa, lien tuc doc radar/am thanh/nhiet do */
    APP_STATE_SCANNING,

    /* ALARM: phat hien nguy co, bat coi va quat muc cao */
    APP_STATE_ALARM
} App_State_t;

/* Anh chup trang thai dau vao da qua cac driver loc nhieu/debounce.
 * Tang application chi xu ly logic, khong doc thanh ghi truc tiep o day.
 */
typedef struct
{
    uint8_t acc_on;
    uint8_t door_open;
    uint8_t locked;
    uint8_t radar_present;
    uint8_t sound_detected;
    float   temperature_c;
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

#define TEMP_THRESHOLD_WARNING_C    25.0f
#define TEMP_THRESHOLD_DANGER_C     32.0f
#define SOUND_THRESHOLD_ADC         180U

/* Gia tri danh dau kenh chap hanh khong su dung.
 * Bang pinout cua nhom khong co servo nen khong duoc gan them chan vat ly.
 */
#define ACTUATOR_UNUSED_PORT        ((GPIO_TypeDef *)0)

/* Cum nut nhan gia lap tin hieu xe.
 * Driver car_state cau hinh cac chan nay la GPIO input pull-up, active-low.
 */
#define CAR_ACC_PORT                GPIOC
#define CAR_ACC_PIN                 0U
#define CAR_DOOR_PORT               GPIOC
#define CAR_DOOR_PIN                1U
#define CAR_LOCK_PORT               GPIOC
#define CAR_LOCK_PIN                2U

/* KY-037 dung ngo ra analog AO dua vao ADC1 channel 0. */
#define SOUND_PORT                  GPIOA
#define SOUND_PIN                   0U
#define SOUND_ADC_CHANNEL           0U

/* BMP/BME280 dung rieng I2C1 tren PB8/PB9. */
#define BMP_I2C_BUS                 I2C1
#define BMP_I2C_SCL_PORT            GPIOB
#define BMP_I2C_SCL_PIN             8U
#define BMP_I2C_SDA_PORT            GPIOB
#define BMP_I2C_SDA_PIN             9U
#define BMP_I2C_SCL_AF              4U
#define BMP_I2C_SDA_AF              4U

/* LCD 16x2 qua PCF8574 dung rieng I2C2 tren PB10/PB11. */
#define LCD_I2C_BUS                 I2C2
#define LCD_I2C_SCL_PORT            GPIOB
#define LCD_I2C_SCL_PIN             10U
#define LCD_I2C_SDA_PORT            GPIOB
#define LCD_I2C_SDA_PIN             3U
#define LCD_I2C_SCL_AF              4U
#define LCD_I2C_SDA_AF              9U

/* TB6612FNG: PA6 xuat PWM TIM3_CH1, PA7/PB0 chon chieu, PB1 standby. */
#define FAN_AIN1_PORT               GPIOA
#define FAN_AIN1_PIN                7U
#define FAN_AIN2_PORT               GPIOB
#define FAN_AIN2_PIN                0U
#define FAN_STBY_PORT               GPIOB
#define FAN_STBY_PIN                1U
#define FAN_PWM_PORT                GPIOA
#define FAN_PWM_PIN                 6U
#define SERVO_PWM_PORT              ACTUATOR_UNUSED_PORT
#define SERVO_PWM_PIN               0U
#define TIM3_AF                     2U

/* Coi chip canh bao, muc 1 la bat. */
#define BUZZER_PORT                 GPIOA
#define BUZZER_PIN                  8U

/* LED rieng bao am thanh vuot nguong. Mac dinh cam LED ngoai vao PB4. */
#define SOUND_LED_PORT              GPIOB
#define SOUND_LED_PIN               4U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
static App_State_t   sg_app_state = APP_STATE_IDLE;
static Cabin_Input_t sg_input = {0U, 0U, 0U, 0U, 0U, 28.0f};

static uint32_t sg_state_tick = 0U;
static uint32_t sg_last_sensor_tick = 0U;
static uint32_t sg_last_temp_tick = 0U;
static uint32_t sg_last_lcd_tick = 0U;
static uint32_t sg_last_sound_tick = 0U;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
static void APP_Init(void);
static void APP_Process(void);
static void APP_ReadFastInputs(void);
static void APP_ReadSlowInputs(void);
static void APP_RunStateMachine(void);
static void APP_UpdateOutputs(void);
static void APP_UpdateDisplay(void);
static uint8_t APP_IsArmedCondition(void);
static uint8_t APP_ShouldAlarm(void);
static uint8_t APP_LcdState(void);
static uint8_t APP_IsSoundIndicatorActive(void);
static void APP_Buzzer_Init(void);
static void APP_Buzzer_Write(uint8_t on);
static void APP_SoundLed_Init(void);
static void APP_SoundLed_Write(uint8_t on);
static void APP_Led_Write(uint8_t on);
static void APP_Led_TogglePattern(uint32_t period_ms);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static void APP_Init(void)
{
    /* Bang cau hinh phan cung duoc nap vao cac driver rieng.
     * Cach nay giu main.c la tang tich hop, con cau hinh thanh ghi nam trong driver.
     */
    Car_Pin_Config_t car_pins[CAR_SIGNAL_MAX] = {
        {CAR_ACC_PORT,  CAR_ACC_PIN},
        {CAR_DOOR_PORT, CAR_DOOR_PIN},
        {CAR_LOCK_PORT, CAR_LOCK_PIN}
    };

    Sound_Config_t sound_config = {
        .port = SOUND_PORT,
        .pin = SOUND_PIN,
        .adcChannel = SOUND_ADC_CHANNEL,
        .adc = ADC1
    };

    Car_Actuator_Config_t actuator_config = {
        .ain1 = {FAN_AIN1_PORT, FAN_AIN1_PIN},
        .ain2 = {FAN_AIN2_PORT, FAN_AIN2_PIN},
        .stby = {FAN_STBY_PORT, FAN_STBY_PIN},
        .pwma = {FAN_PWM_PORT, FAN_PWM_PIN, TIM3_AF},

        /* Project hien tai khong lap servo. Driver se bo qua kenh nay. */
        .servo = {SERVO_PWM_PORT, SERVO_PWM_PIN, TIM3_AF}
    };

    SysTimer_Init(SystemCoreClock);
    APP_Buzzer_Init();
    APP_SoundLed_Init();

    Car_State_Init(car_pins);
    Car_Actuator_Init(&actuator_config);

    /* Khoi tao 2 bus I2C doc lap:
     * - BMP/BME280 tren I2C1
     * - LCD/PCF8574 tren I2C2
     */
    HW_GPIO_Init_I2C_Pin(BMP_I2C_SCL_PORT, BMP_I2C_SCL_PIN, BMP_I2C_SCL_AF);
    HW_GPIO_Init_I2C_Pin(BMP_I2C_SDA_PORT, BMP_I2C_SDA_PIN, BMP_I2C_SDA_AF);
    HW_I2C_Init(BMP_I2C_BUS);
    BMP280_Init(BMP_I2C_BUS);

    HW_GPIO_Init_I2C_Pin(LCD_I2C_SCL_PORT, LCD_I2C_SCL_PIN, LCD_I2C_SCL_AF);
    HW_GPIO_Init_I2C_Pin(LCD_I2C_SDA_PORT, LCD_I2C_SDA_PIN, LCD_I2C_SDA_AF);
    HW_I2C_Init(LCD_I2C_BUS);
    LCD_Init(LCD_I2C_BUS);

    Radar_EXTI_Init();
    Sound_Init(&sound_config);
    Sound_SetThreshold(SOUND_THRESHOLD_ADC);

    Actuator_Set_Fan_Speed(0U);
    APP_Buzzer_Write(0U);
    APP_Led_Write(0U);
    APP_SoundLed_Write(0U);

    sg_state_tick = SysTimer_GetTick();
}

static void APP_Process(void)
{
    uint32_t now = SysTimer_GetTick();

    /* Xu ly ADC am thanh cang thuong xuyen cang tot de bo loc moving average
     * co mau moi, nhung khong dung delay/blocking trong vong lap chinh.
     */
    Sound_Process();

    /* Nhom tin hieu nhanh: nut nhan, radar, am thanh va state machine. */
    if ((now - sg_last_sensor_tick) >= APP_SENSOR_PERIOD_MS)
    {
        sg_last_sensor_tick = now;
        APP_ReadFastInputs();
        APP_RunStateMachine();
        APP_UpdateOutputs();
    }

    /* Nhiet do/I2C doc cham hon de tranh chiem bus va lam tre vong quet nhanh. */
    if ((now - sg_last_temp_tick) >= APP_TEMP_PERIOD_MS)
    {
        sg_last_temp_tick = now;
        APP_ReadSlowInputs();
    }

    /* LCD cap nhat 500ms/lap de tranh nhap nhay va giam so lan clear man hinh. */
    if ((now - sg_last_lcd_tick) >= APP_LCD_PERIOD_MS)
    {
        sg_last_lcd_tick = now;
        APP_UpdateDisplay();
    }
}

static void APP_ReadFastInputs(void)
{
    /* TẠM THỜI: Đọc trực tiếp nút bấm B1 (PC13) có sẵn trên kit Nucleo */

    /* ÉP LOGIC:
     * - Khi NHẤN GIỮ nút B1 (b1_status == 0): Coi như xe ĐÃ KHÓA (locked = 1U)
     * - Khi THẢ nút B1 (b1_status == 1): Coi như xe MỞ KHÓA (locked = 0U)
     */

    /* Các tín hiệu giả lập khác ép bằng 0 để không bị vướng điều kiện */
    sg_input.acc_on = 0U;     // Giả lập xe luôn tắt máy
    sg_input.door_open = 0U;   // Giả lập cửa luôn đóng

    /* Đọc dữ liệu từ các cảm biến (Giữ nguyên) */
    sg_input.acc_on = Car_Get_System_Status(CAR_ACC_SIGNAL);
    sg_input.door_open = Car_Get_System_Status(CAR_DOOR_SIGNAL);
    sg_input.locked = Car_Get_System_Status(CAR_LOCK_SIGNAL);
    sg_input.radar_present = Radar_Is_Detected();
    sg_input.sound_detected = Sound_IsDetected();
    if (((sg_app_state == APP_STATE_SCANNING) ||
         (sg_app_state == APP_STATE_ALARM)) &&
        (sg_input.sound_detected != 0U))
    {
        sg_last_sound_tick = SysTimer_GetTick();
    }
}

static void APP_ReadSlowInputs(void)
{
    sg_input.temperature_c = BMP280_Read_Temperature(BMP_I2C_BUS);
}

static void APP_RunStateMachine(void)
{
    uint32_t now = SysTimer_GetTick();

    /* May trang thai tong the:
     * IDLE -> ARMING -> SCANNING -> ALARM.
     * Neu ACC bat, cua mo, hoac xe khong khoa thi quay ve IDLE ngay.
     */
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
            if (APP_IsArmedCondition() == 0U)
            {
                sg_app_state = APP_STATE_IDLE;
                sg_state_tick = now;
            }
            else if ((now - sg_state_tick) >= APP_ARMING_TIME_MS)
            {
                sg_app_state = APP_STATE_SCANNING;
                sg_state_tick = now;
            }
            break;

        case APP_STATE_SCANNING:
            if (APP_IsArmedCondition() == 0U)
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
            if (APP_IsArmedCondition() == 0U)
            {
                sg_app_state = APP_STATE_IDLE;
                sg_state_tick = now;
                Radar_ClearPresence();
            }
            else
            {
                /* Khi coi dang keu, radar co the bi nhieu/giu co.
                 * Xoa latch radar trong ALARM de khong bi ket bao dong chi vi radar.
                 */
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

static void APP_UpdateDisplay(void)
{
    LCD_Display_Status(LCD_I2C_BUS, APP_LcdState(), sg_input.temperature_c);
}

static uint8_t APP_IsArmedCondition(void)
{
    /* Dieu kien bao ve: xe tat ACC, cua dong, va da khoa.
     * Cac gia tri nay da duoc driver car_state doi tu active-low sang logic 1/0.
     */
    return ((sg_input.acc_on == 0U) &&
            (sg_input.door_open == 0U) &&
            (sg_input.locked != 0U)) ? 1U : 0U;
}

static uint8_t APP_ShouldAlarm(void)
{
    /* Nhiet do nguy hiem hoac tieng keu lon duoc phep kich ALARM doc lap.
     * Radar ket hop voi nhiet do canh bao de giam false positive khi chi co chuyen dong.
     */
    if (sg_input.temperature_c >= TEMP_THRESHOLD_DANGER_C)
    {
        return 1U;
    }

    if (sg_input.sound_detected != 0U)
    {
        return 1U;
    }

    if ((sg_input.radar_present == RADAR_PRESENCE) &&
        (sg_input.temperature_c >= TEMP_THRESHOLD_WARNING_C))
    {
        return 1U;
    }

    return 0U;
}

static uint8_t APP_LcdState(void)
{
    if (sg_app_state == APP_STATE_ALARM)
    {
        return SYS_STATE_DANGER;
    }

    if (sg_app_state == APP_STATE_ARMING)
    {
        return SYS_STATE_WARNING;
    }

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

static uint8_t APP_IsSoundIndicatorActive(void)
{
    if (sg_last_sound_tick == 0U)
    {
        return 0U;
    }

    return ((SysTimer_GetTick() - sg_last_sound_tick) < APP_SOUND_INDICATOR_HOLD_MS) ? 1U : 0U;
}

static void APP_Buzzer_Init(void)
{
    /* Cau hinh PA8 bang thanh ghi truc tiep: output push-pull, no pull. */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    BUZZER_PORT->MODER &= ~(3U << (BUZZER_PIN * 2U));
    BUZZER_PORT->MODER |=  (1U << (BUZZER_PIN * 2U));
    BUZZER_PORT->OTYPER &= ~(1U << BUZZER_PIN);
    BUZZER_PORT->PUPDR &= ~(3U << (BUZZER_PIN * 2U));
    BUZZER_PORT->OSPEEDR &= ~(3U << (BUZZER_PIN * 2U));
    BUZZER_PORT->ODR &= ~(1U << BUZZER_PIN);
}

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

static void APP_SoundLed_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    SOUND_LED_PORT->MODER &= ~(3U << (SOUND_LED_PIN * 2U));
    SOUND_LED_PORT->MODER |=  (1U << (SOUND_LED_PIN * 2U));
    SOUND_LED_PORT->OTYPER &= ~(1U << SOUND_LED_PIN);
    SOUND_LED_PORT->PUPDR &= ~(3U << (SOUND_LED_PIN * 2U));
    SOUND_LED_PORT->OSPEEDR &= ~(3U << (SOUND_LED_PIN * 2U));
    SOUND_LED_PORT->ODR &= ~(1U << SOUND_LED_PIN);
}

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

  /* USER CODE END Init */

  /* Giu clock mac dinh HSI 16 MHz de khop voi driver hw_i2c.c.
   * Driver I2C hien tai cau hinh CR2/CCR/TRISE co dinh cho APB1 = 16 MHz.
   */
  /* SystemClock_Config(); */

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
    APP_Init();
  /* USER CODE END 2 */

  /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        APP_Process();
    }
    /* USER CODE END 3 */
}
/**
  * @brief System Clock Configuration
  * @retval None
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
