/* stm32f767_min.h - minimal bare-metal register map for STM32F767ZI
 * Only what the LED + TIM1 PWM examples need. No CMSIS, no HAL.
 * Reference: RM0410 (STM32F76x/77x reference manual)
 */
#ifndef STM32F767_MIN_H
#define STM32F767_MIN_H

#include <stdint.h>

#define REG32(addr) (*(volatile uint32_t *)(addr))

/* ---------------- Bus / peripheral base addresses ---------------- */
#define PERIPH_BASE   0x40000000UL
#define APB2PERIPH    (PERIPH_BASE + 0x00010000UL)
#define AHB1PERIPH    (PERIPH_BASE + 0x00020000UL)

#define GPIOA_BASE    (AHB1PERIPH + 0x0000UL)
#define GPIOB_BASE    (AHB1PERIPH + 0x0400UL)
#define GPIOE_BASE    (AHB1PERIPH + 0x1000UL)
#define RCC_BASE      (AHB1PERIPH + 0x3800UL)
#define TIM1_BASE     (APB2PERIPH + 0x0000UL)
#define I2C1_BASE     0x4000 5400

/* ---------------- RCC ---------------- */
#define RCC_AHB1ENR   REG32(RCC_BASE + 0x30)
#define RCC_APB2ENR   REG32(RCC_BASE + 0x44)

#define RCC_AHB1ENR_GPIOAEN  (1UL << 0)
#define RCC_AHB1ENR_GPIOBEN  (1UL << 1)
#define RCC_AHB1ENR_GPIOEEN  (1UL << 4)
#define RCC_APB2ENR_TIM1EN   (1UL << 0)

/* ---------------- GPIO (offsets are the same for every port) ------ */
#define GPIO_MODER(p)    REG32((p) + 0x00)  /* 00 in, 01 out, 10 AF, 11 analog */
#define GPIO_OTYPER(p)   REG32((p) + 0x04)  /* 0 push-pull, 1 open-drain       */
#define GPIO_OSPEEDR(p)  REG32((p) + 0x08)  /* 00 low .. 11 very high          */
#define GPIO_PUPDR(p)    REG32((p) + 0x0C)  /* 00 none, 01 PU, 10 PD           */
#define GPIO_IDR(p)      REG32((p) + 0x10)
#define GPIO_ODR(p)      REG32((p) + 0x14)
#define GPIO_BSRR(p)     REG32((p) + 0x18)  /* [15:0] set, [31:16] reset       */
#define GPIO_AFRL(p)     REG32((p) + 0x20)  /* pins 0..7                       */
#define GPIO_AFRH(p)     REG32((p) + 0x24)  /* pins 8..15                      */

/* ---------------- TIM1 (advanced-control timer) ------------------- */
#define TIM1_CR1     REG32(TIM1_BASE + 0x00)
#define TIM1_CR2     REG32(TIM1_BASE + 0x04)
#define TIM1_DIER    REG32(TIM1_BASE + 0x0C)
#define TIM1_SR      REG32(TIM1_BASE + 0x10)
#define TIM1_EGR     REG32(TIM1_BASE + 0x14)
#define TIM1_CCMR1   REG32(TIM1_BASE + 0x18)
#define TIM1_CCER    REG32(TIM1_BASE + 0x20)
#define TIM1_CNT     REG32(TIM1_BASE + 0x24)
#define TIM1_PSC     REG32(TIM1_BASE + 0x28)
#define TIM1_ARR     REG32(TIM1_BASE + 0x2C)
#define TIM1_RCR     REG32(TIM1_BASE + 0x30)
#define TIM1_CCR1    REG32(TIM1_BASE + 0x34)
#define TIM1_BDTR    REG32(TIM1_BASE + 0x44)

#define TIM_CR1_CEN     (1UL << 0)
#define TIM_CR1_ARPE    (1UL << 7)
#define TIM_EGR_UG      (1UL << 0)
#define TIM_CCMR1_OC1PE (1UL << 3)
#define TIM_CCMR1_OC1M_PWM1 (6UL << 4)   /* 110 = PWM mode 1 */
#define TIM_CCER_CC1E   (1UL << 0)
#define TIM_CCER_CC1P   (1UL << 1)
#define TIM_BDTR_MOE    (1UL << 15)      /* advanced timers only - easy to forget */

//I2C reg
#define I2C_CR1     REG32(I2C1_BASE + 0x00)
#define I2C_CR2     REG32(I2C1_BASE + 0x04)
#define I2C_OAR1    REG32(I2C1_BASE + 0x08)
#define I2C_OAR2    REG32(I2C1_BASE + 0x0C)
#define I2C_TIMINGR     REG32(I2C1_BASE + 0x10)
#define I2C_TIMEOUTR     REG32(I2C1_BASE + 0x14)
#define I2C_ISR     REG32(I2C1_BASE + 0x18)
#define I2C_ICR     REG32(I2C1_BASE + 0x1C)
#define I2C_PECR    REG32(I2C1_BASE + 0x20)
#define I2C_RXDR    REG32(I2C1_BASE + 0x24)
#define I2C_TXDR    REG32(I2C1_BASE + 0x28)



/* ---------------- SysTick (in the Cortex-M7 core) ----------------- */
#define SYST_CSR   REG32(0xE000E010)
#define SYST_RVR   REG32(0xE000E014)
#define SYST_CVR   REG32(0xE000E018)

#define SYST_CSR_ENABLE     (1UL << 0)
#define SYST_CSR_CLKSOURCE  (1UL << 2)   /* 1 = processor clock */
#define SYST_CSR_COUNTFLAG  (1UL << 16)  /* cleared when read   */

/* After reset the CPU runs from HSI = 16 MHz. No PLL setup here on purpose. */
#define SYSCLK_HZ 16000000UL

static inline void systick_init(void)
{
    SYST_RVR = (SYSCLK_HZ / 1000UL) - 1UL;   /* 1 ms period */
    SYST_CVR = 0;
    SYST_CSR = SYST_CSR_CLKSOURCE | SYST_CSR_ENABLE;  /* polled, no interrupt */
}

static inline void delay_ms(uint32_t ms)
{
    (void)SYST_CSR;                     /* clear a stale COUNTFLAG */
    while (ms--) {
        while (!(SYST_CSR & SYST_CSR_COUNTFLAG))
            ;
    }
}

#endif /* STM32F767_MIN_H */
