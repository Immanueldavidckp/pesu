/*
 * C2000 (F280049C) DAC waveforms: triangle, sine, staircase, ramp
 * Set WAVE to the program you want to build.
 */
#define WAVE_TRIANGLE  1
#define WAVE_SINE      2
#define WAVE_STAIR     3
#define WAVE_RAMP      4

#define WAVE           WAVE_TRIANGLE

/* ---------------------------------------------------------------- */
#if WAVE == WAVE_TRIANGLE   /* 1) TRIANGLE */

#include "driverlib.h"
#include "device.h"
#include "board.h"

int32_t dacValue = 0;
int16_t dir = 1;

void main(void)
{
    Device_init();
    Device_initGPIO();
    Interrupt_initModule();
    Interrupt_initVectorTable();
    Board_init();

    while(1)
    {
        DAC_setShadowValue(DAC0_BASE, (uint16_t)dacValue);

        dacValue += dir * 32;

        if(dacValue >= 4095) { dacValue = 4095; dir = -1; }
        if(dacValue <= 0)    { dacValue = 0; dir = 1; }

        DEVICE_DELAY_US(25);
    }
}

/* ---------------------------------------------------------------- */
#elif WAVE == WAVE_SINE     /* 2) SINUSOIDAL */

#include "driverlib.h"
#include "device.h"
#include "board.h"
#include <math.h>

void main(void)
{
    Device_init();
    Device_initGPIO();
    Interrupt_initModule();
    Interrupt_initVectorTable();
    Board_init();

    uint16_t i;

    while(1)
    {
        for(i = 0; i < 256; i++)
        {
            DAC_setShadowValue(DAC0_BASE,
                (uint16_t)(2048 + 2047 *
                sinf(6.2831853f * i / 256)));

            DEVICE_DELAY_US(25);
        }
    }
}

/* ---------------------------------------------------------------- */
#elif WAVE == WAVE_STAIR    /* 3) STAIRCASE */

#include "driverlib.h"
#include "device.h"
#include "board.h"

int32_t dacValue = 0;
int16_t dir = 1;
void main(void)
{
    Device_init();
    Device_initGPIO();
    Interrupt_initModule();
    Interrupt_initVectorTable();
    Board_init();

    while(1)
    {
        DAC_setShadowValue(DAC0_BASE, dacValue);
        dacValue += dir * 256;

        if(dacValue >= 4095) { dacValue = 4095; dir = -1; }
        if(dacValue <= 0)    { dacValue = 0; dir = 1; }

        DEVICE_DELAY_US(500);
    }
}

/* ---------------------------------------------------------------- */
#elif WAVE == WAVE_RAMP     /* 4) RAMP */

#include "driverlib.h"
#include "device.h"
#include "board.h"

uint16_t dacValue = 0;

void main(void)
{
    Device_init();
    Device_initGPIO();
    Interrupt_initModule();
    Interrupt_initVectorTable();
    Board_init();

    while(1)
    {
        DAC_setShadowValue(DAC0_BASE, dacValue);
        dacValue += 4;
        if(dacValue >= 4095) dacValue = 0;
        DEVICE_DELAY_US(10);
    }
}

#endif
