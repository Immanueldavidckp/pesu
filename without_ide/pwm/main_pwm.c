/* main_pwm.c - 1 kHz PWM on TIM1_CH1 -> PE9 (Arduino header D6 on Nucleo-144)
 *
 * Put a scope on D6, or an LED + ~330R from D6 to GND, and you get a
 * breathing ramp.
 *
 * Timer maths, default HSI = 16 MHz, APB2 prescaler = 1 so TIM1 clk = 16 MHz:
 *
 *     tick     = TIMclk / (PSC + 1) = 16 MHz / 16   = 1 MHz  (1 us)
 *     f_pwm    = tick   / (ARR + 1) = 1 MHz / 1000  = 1 kHz
 *     duty [%] = CCR1 / (ARR + 1) * 100
 *
 * PE9 alternate function for TIM1_CH1 is AF1 (datasheet Table 12).
 * If you prefer PA8, it is also AF1 - just swap the port and pin below.
 */

#include "stm32f767_min.h"

#define PWM_PIN   9u          /* PE9 */
#define PWM_PORT  GPIOE_BASE
#define PWM_AF    1u          /* AF1 = TIM1_CH1 */

#define PWM_ARR   999u        /* 1000 counts per period */

static void pwm_gpio_init(void)
{
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOEEN;
    (void)RCC_AHB1ENR;

    /* MODER = 10 (alternate function) */
    GPIO_MODER(PWM_PORT) &= ~(3UL << (PWM_PIN * 2));
    GPIO_MODER(PWM_PORT) |=  (2UL << (PWM_PIN * 2));

    /* Push-pull, no pull, high speed (edges stay clean at higher f_pwm) */
    GPIO_OTYPER(PWM_PORT)  &= ~(1UL << PWM_PIN);
    GPIO_PUPDR(PWM_PORT)   &= ~(3UL << (PWM_PIN * 2));
    GPIO_OSPEEDR(PWM_PORT) |=  (3UL << (PWM_PIN * 2));

    /* Pin 9 lives in AFRH, 4 bits per pin, at position (9 - 8) * 4 = 4 */
    GPIO_AFRH(PWM_PORT) &= ~(0xFUL << ((PWM_PIN - 8) * 4));
    GPIO_AFRH(PWM_PORT) |=  ((uint32_t)PWM_AF << ((PWM_PIN - 8) * 4));
}

static void tim1_pwm_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_TIM1EN;
    (void)RCC_APB2ENR;

    TIM1_CR1 = 0;                 /* stop the timer while we configure it */

    TIM1_PSC = 16 - 1;            /* 1 MHz counter tick */
    TIM1_ARR = PWM_ARR;           /* 1 kHz PWM          */
    TIM1_RCR = 0;

    /* Channel 1: PWM mode 1 (output high while CNT < CCR1) + preload on CCR1
     * so a mid-period duty write only takes effect at the next update. */
    TIM1_CCMR1 = TIM_CCMR1_OC1M_PWM1 | TIM_CCMR1_OC1PE;

    TIM1_CCR1  = 0;               /* start at 0 % duty */

    TIM1_CCER  = TIM_CCER_CC1E;   /* enable OC1 output, active high */

    TIM1_CR1  |= TIM_CR1_ARPE;    /* buffer ARR too */

    /* TIM1 is an ADVANCED timer: without MOE the pin stays dead no matter
     * how correct everything else is. This is the classic gotcha. */
    TIM1_BDTR |= TIM_BDTR_MOE;

    TIM1_EGR   = TIM_EGR_UG;      /* force an update: loads PSC/ARR/CCR1 */
    TIM1_SR    = 0;               /* clear the flag that UG just set     */

    TIM1_CR1  |= TIM_CR1_CEN;     /* go */
}

static inline void pwm_set_duty(uint32_t permille)   /* 0 .. 1000 */
{
    if (permille > PWM_ARR + 1u)
        permille = PWM_ARR + 1u;
    TIM1_CCR1 = permille;
}

int main(void)
{
    systick_init();
    pwm_gpio_init();
    tim1_pwm_init();

    while (1) {
        for (uint32_t d = 0; d <= 1000; d += 10) {   /* fade up   */
            pwm_set_duty(d);
            delay_ms(10);
        }
        for (uint32_t d = 1000; d > 0; d -= 10) {    /* fade down */
            pwm_set_duty(d);
            delay_ms(10);
        }
    }
}
