/* startup.c - reset vector + C runtime bring-up for STM32F767ZI (Cortex-M7) */

#include <stdint.h>

/* Symbols provided by linker.ld */
extern uint32_t _sidata;   /* .data image start, in flash   */
extern uint32_t _sdata;    /* .data start, in RAM           */
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;
extern uint32_t _estack;   /* top of RAM = initial MSP      */

int main(void);

void Reset_Handler(void);
void Default_Handler(void);

#define WEAK_DEFAULT __attribute__((weak, alias("Default_Handler")))

void NMI_Handler(void)        WEAK_DEFAULT;
void HardFault_Handler(void)  WEAK_DEFAULT;
void MemManage_Handler(void)  WEAK_DEFAULT;
void BusFault_Handler(void)   WEAK_DEFAULT;
void UsageFault_Handler(void) WEAK_DEFAULT;
void SVC_Handler(void)        WEAK_DEFAULT;
void DebugMon_Handler(void)   WEAK_DEFAULT;
void PendSV_Handler(void)     WEAK_DEFAULT;
void SysTick_Handler(void)    WEAK_DEFAULT;

/* Core vector table only (entries 0..15). We use no peripheral interrupts.
 * If you later enable one (e.g. TIM1_UP), you must append the peripheral
 * vectors here in the exact order given in RM0410 Table 47. */
__attribute__((section(".isr_vector"), used))
void (* const g_pfnVectors[])(void) = {
    (void (*)(void))&_estack,   /*  0: initial stack pointer */
    Reset_Handler,              /*  1: reset                 */
    NMI_Handler,                /*  2                        */
    HardFault_Handler,          /*  3                        */
    MemManage_Handler,          /*  4                        */
    BusFault_Handler,           /*  5                        */
    UsageFault_Handler,         /*  6                        */
    0, 0, 0, 0,                 /*  7-10: reserved           */
    SVC_Handler,                /* 11                        */
    DebugMon_Handler,           /* 12                        */
    0,                          /* 13: reserved              */
    PendSV_Handler,             /* 14                        */
    SysTick_Handler,            /* 15                        */
};

#define SCB_VTOR   (*(volatile uint32_t *)0xE000ED08)
#define SCB_CPACR  (*(volatile uint32_t *)0xE000ED88)

void Reset_Handler(void)
{
    /* Point the vector table at our table in flash. */
    SCB_VTOR = 0x08000000UL;

    /* Enable CP10/CP11 (the FPU) before any float instruction runs.
     * Required because we build with -mfloat-abi=hard. */
    SCB_CPACR |= (0xFUL << 20);
    __asm volatile ("dsb");
    __asm volatile ("isb");

    /* Copy initialised data from flash to RAM. */
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;
    while (dst < &_edata)
        *dst++ = *src++;

    /* Zero the .bss section. */
    for (dst = &_sbss; dst < &_ebss; dst++)
        *dst = 0;

    main();

    while (1)
        ;   /* main() should never return */
}

void Default_Handler(void)
{
    while (1)
        ;   /* park here so a stray fault is obvious in the debugger */
}
