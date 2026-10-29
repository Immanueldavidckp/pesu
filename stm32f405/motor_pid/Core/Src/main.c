/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : JAN-KAI Motor Training Kit - Experiment 01
  *                   Open-loop six-step BLDC commutation using TIM1 and Hall sensors
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct
{
    int8_t U;
    int8_t V;
    int8_t W;
} Step; /* 1 = PWM high-side, 0 = low-side ON, -1 = phase OFF */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DUTY_INIT_PCT     30UL

/* Closed-loop speed PI controller */
#define KP                0.05f
#define KI                0.008f
#define SPEED_REF_RPM     1500.0f
#define CTRL_MS           10U
#define POLE_PAIRS_F      4.0f
#define HALL_EDGES_REV    6.0f
#define HALL_TIMEOUT_MS   250U

/* Clock (CubeMX): HSE 8 MHz -> PLL M4 N80 P2 -> SYSCLK 80 MHz, APB2 /1,
 * so TIM1CLK = 80 MHz on STM32F405RG.
 * TIM1 (CubeMX): PSC 7, ARR 999 -> 10 kHz PWM; DeadTime 40 ticks -> 500 ns.
 * The code reads ARR and SystemCoreClock at run time, so a clock change in
 * CubeMX needs no edit here.
 */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static volatile uint32_t g_cmp = 0U;
static volatile uint8_t g_last_hall = 0xFFU;
static float g_integral = 0.0f;
static float g_duty = 0.30f;
static volatile uint32_t g_hall_us = 0U;
static volatile uint32_t g_last_cyc = 0U;
static volatile uint32_t g_last_hall_tick = 0U;
static uint32_t g_cycles_per_us = 0U;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void Apply(uint8_t hall);
static uint8_t ReadHall(void);
static void SetPhase(uint32_t channel, int8_t state);
static void MotorOutputsDisable(void);
static float Speed_RPM(void);
static float PI_Run(float ref, float meas, float ts);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* Six-step commutation table indexed by Hall state [HC HB HA] represented
 * as bit 2 = PA2, bit 1 = PA1, bit 0 = PA0.
 * Verify the phase sequence against the specific motor and Hall wiring
 * before applying power. The supplied table is preserved from the request.
 */
static const Step TAB[8] = {
    { 0,  0,  0}, /* 000: invalid Hall state */
    { 1,  0, -1}, /* 001 */
    {-1,  1,  0}, /* 010 */
    { 0,  1, -1}, /* 011 */
    { 0, -1,  1}, /* 100 */
    { 1, -1,  0}, /* 101 */
    {-1,  0,  1}, /* 110 */
    { 0,  0,  0}  /* 111: invalid Hall state */
};

static void MotorOutputsDisable(void)
{
    /* Disable all six timer outputs before changing the phase pattern. */
    CLEAR_BIT(TIM1->CCER, TIM_CCER_CC1E | TIM_CCER_CC1NE |
                          TIM_CCER_CC2E | TIM_CCER_CC2NE |
                          TIM_CCER_CC3E | TIM_CCER_CC3NE);
}

static void SetPhase(uint32_t channel, int8_t state)
{
    uint32_t main_enable = 0U;
    uint32_t comp_enable = 0U;
    uint32_t ccr_value = 0U;

    switch (channel)
    {
        case TIM_CHANNEL_1:
            main_enable = TIM_CCER_CC1E;
            comp_enable = TIM_CCER_CC1NE;
            break;
        case TIM_CHANNEL_2:
            main_enable = TIM_CCER_CC2E;
            comp_enable = TIM_CCER_CC2NE;
            break;
        case TIM_CHANNEL_3:
            main_enable = TIM_CCER_CC3E;
            comp_enable = TIM_CCER_CC3NE;
            break;
        default:
            return;
    }

    /* Both outputs for this phase remain disabled unless selected below. */
    CLEAR_BIT(TIM1->CCER, main_enable | comp_enable);

    if (state == 1)
    {
        ccr_value = g_cmp;
        if (channel == TIM_CHANNEL_1) __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, ccr_value);
        if (channel == TIM_CHANNEL_2) __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, ccr_value);
        if (channel == TIM_CHANNEL_3) __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, ccr_value);
        SET_BIT(TIM1->CCER, main_enable); /* PWM high-side output */
    }
    else if (state == 0)
    {
        /* CCR=0 makes OCREF inactive in PWM mode 1; the complementary
         * output is used as the low-side static output. */
        if (channel == TIM_CHANNEL_1) __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0U);
        if (channel == TIM_CHANNEL_2) __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0U);
        if (channel == TIM_CHANNEL_3) __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 0U);
        SET_BIT(TIM1->CCER, comp_enable);
    }
    /* state == -1: leave both outputs disabled (phase floating). */
}

static void Apply(uint8_t hall)
{
    if ((hall == 0U) || (hall == 7U))
    {
        MotorOutputsDisable();
        g_last_hall = hall;
        return;
    }

    /* Disable all outputs while changing commutation states. For real power
     * hardware, confirm gate-driver interlocking and switching dead-time on
     * the oscilloscope before enabling the DC bus. */
    MotorOutputsDisable();

    SetPhase(TIM_CHANNEL_1, TAB[hall].U);
    SetPhase(TIM_CHANNEL_2, TAB[hall].V);
    SetPhase(TIM_CHANNEL_3, TAB[hall].W);

    g_last_hall = hall;
}

static uint8_t ReadHall(void)
{
    uint8_t hall = 0U;

    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) hall |= 0x01U;
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_SET) hall |= 0x02U;
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2) == GPIO_PIN_SET) hall |= 0x04U;

    return hall;
}

static float Speed_RPM(void)
{
    uint32_t interval_us;
    uint32_t last_tick;

    /* Copy volatile values once so the calculation uses a consistent snapshot. */
    interval_us = g_hall_us;
    last_tick = g_last_hall_tick;

    if ((interval_us == 0U) || ((HAL_GetTick() - last_tick) > HALL_TIMEOUT_MS))
    {
        return 0.0f;
    }

    /* Six Hall edges per electrical revolution, POLE_PAIRS_F per mechanical rev. */
    {
        float f_elec = 1000000.0f / ((float)interval_us * HALL_EDGES_REV);
        return (f_elec / POLE_PAIRS_F) * 60.0f;
    }
}

static float PI_Run(float ref, float meas, float ts)
{
    float err = ref - meas;
    float out;

    g_integral += KI * err * ts;
    if (g_integral > 1.0f)  g_integral = 1.0f;
    if (g_integral < -0.1f) g_integral = -0.1f;

    out = KP * err + g_integral;
    if (out > 1.0f) out = 1.0f;
    if (out < 0.0f) out = 0.0f;
    return out;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if ((GPIO_Pin == GPIO_PIN_0) ||
        (GPIO_Pin == GPIO_PIN_1) ||
        (GPIO_Pin == GPIO_PIN_2))
    {
        uint32_t now_cyc = DWT->CYCCNT;
        uint32_t now_tick = HAL_GetTick();

        if (g_last_cyc != 0U)
        {
            uint32_t elapsed_cycles = now_cyc - g_last_cyc; /* wrap-safe */
            if (g_cycles_per_us != 0U)
            {
                g_hall_us = elapsed_cycles / g_cycles_per_us;
            }
        }

        g_last_cyc = now_cyc;
        g_last_hall_tick = now_tick;
        Apply(ReadHall());
    }
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

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
    /* DWT cycle counter times the Hall edges; it is off after reset. */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    g_cycles_per_us = SystemCoreClock / 1000000UL;

    g_cmp = (TIM1->ARR * DUTY_INIT_PCT) / 100UL;

    /* Start timer channels; individual output enables are controlled by Apply(). */
    if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1) != HAL_OK) Error_Handler();
    if (HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1) != HAL_OK) Error_Handler();
    if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2) != HAL_OK) Error_Handler();
    if (HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2) != HAL_OK) Error_Handler();
    if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3) != HAL_OK) Error_Handler();
    if (HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_3) != HAL_OK) Error_Handler();

    __HAL_TIM_MOE_ENABLE(&htim1);
    Apply(ReadHall());
    g_last_hall_tick = HAL_GetTick();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    uint32_t t_last = HAL_GetTick();
    while (1)
    {
        uint32_t now = HAL_GetTick();
        if ((now - t_last) >= CTRL_MS)
        {
            float dt = (float)(now - t_last) / 1000.0f;
            float speed_rpm;
            t_last = now;

            speed_rpm = Speed_RPM();
            g_duty = PI_Run(SPEED_REF_RPM, speed_rpm, dt);

            if (g_duty > 1.0f) g_duty = 1.0f;
            if (g_duty < 0.0f) g_duty = 0.0f;
            g_cmp = (uint32_t)((float)TIM1->ARR * g_duty);
            if (g_cmp > TIM1->ARR) g_cmp = TIM1->ARR;
        }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 80;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
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
#ifdef USE_FULL_ASSERT
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
