/* main_led.c - blink the three user LEDs on the NUCLEO-F767ZI
 *
 *   LD1 green -> PB0
 *   LD2 blue  -> PB7
 *   LD3 red   -> PB14
 *
 * Clock: default HSI, 16 MHz. No PLL configuration at all.
 */

#include "stm32f767_min.h"

#define LD1  0u
#define LD2  7u
#define LD3  14u

static void led_init(void)
{
    /* 1. Clock the GPIOB port. Nothing in a peripheral responds until you
     *    do this, and the write needs a moment to take effect. */
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    (void)RCC_AHB1ENR;          /* read back: guarantees the write landed */

    /* 2. MODER: 2 bits per pin. Clear then set 01 = general purpose output. */
    GPIO_MODER(GPIOB_BASE) &= ~((3UL << (LD1 * 2)) |
                                (3UL << (LD2 * 2)) |
                                (3UL << (LD3 * 2)));
    GPIO_MODER(GPIOB_BASE) |=  ((1UL << (LD1 * 2)) |
                                (1UL << (LD2 * 2)) |
                                (1UL << (LD3 * 2)));

    /* Push-pull + low speed are the reset defaults, so OTYPER/OSPEEDR/PUPDR
     * need no changes for an LED. */
}

/* BSRR is the atomic way to drive a pin: low half sets, high half resets.
 * No read-modify-write on ODR, so no race with an interrupt. */
static inline void led_on(uint32_t pin)     { GPIO_BSRR(GPIOB_BASE) = (1UL << pin); }
static inline void led_off(uint32_t pin)    { GPIO_BSRR(GPIOB_BASE) = (1UL << (pin + 16)); }
static inline void led_toggle(uint32_t pin) { GPIO_ODR(GPIOB_BASE) ^= (1UL << pin); }

int main(void)
{
    systick_init();
    led_init();

    while (1) {
        /* Chase the three LEDs, then blink all together twice. */
        led_on(LD1);  delay_ms(200); led_off(LD1);
        led_on(LD2);  delay_ms(200); led_off(LD2);
        led_on(LD3);  delay_ms(200); led_off(LD3);

        for (int i = 0; i < 4; i++) {
            led_toggle(LD1);
            led_toggle(LD2);
            led_toggle(LD3);
            delay_ms(150);
        }
    }
}
