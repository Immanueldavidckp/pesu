//#############################################################################
// lab_main.c  -  ePWM phase shift demo for TMS320F280049C (LAUNCHXL-F280049C)
//
// ePWM1 = MASTER, ePWM2 = SLAVE (phase-shifted from ePWM1)
//   ePWM1A -> GPIO0
//   ePWM2A -> GPIO2
//
// Both: 10 kHz, 50 % duty, up-count mode.
// ePWM2 LAGS ePWM1 by 'phaseDeg' degrees.
// Add 'phaseDeg' to the CCS Expressions window and change it while the
// code is running - the ePWM2 waveform moves on the scope live.
//#############################################################################

#include "driverlib.h"
#include "device.h"

//-----------------------------------------------------------------------------
// Timing
//   EPWMCLK = SYSCLK / 2 = 100 MHz / 2 = 50 MHz
//   TBCLK   = EPWMCLK / (CLKDIV * HSPCLKDIV) = 50 MHz / (1 * 1) = 50 MHz
//   Up-count mode: period = (TBPRD + 1) TBCLK cycles
//   5000 cycles * 20 ns = 100 us  ->  10 kHz
//-----------------------------------------------------------------------------
#define EPWM_PERIOD_COUNTS   5000U                      // counts per PWM period
#define EPWM_TBPRD           (EPWM_PERIOD_COUNTS - 1U)  // value written to TBPRD
#define EPWM_DUTY_CMPA       (EPWM_PERIOD_COUNTS / 2U)  // 50 % duty

//-----------------------------------------------------------------------------
// Globals (watch / edit these in CCS Expressions window)
//-----------------------------------------------------------------------------
volatile uint16_t phaseDeg        = 90U;     // desired ePWM2 lag, 0..359 deg
volatile uint16_t phaseCountsTBPHS = 0U;     // what actually went into TBPHS
static   uint16_t appliedPhaseDeg = 0xFFFFU; // last value applied

//-----------------------------------------------------------------------------
// Prototypes
//-----------------------------------------------------------------------------
static void initEPWMPins(void);
static void initEPWMCommon(uint32_t base);
static void setSlavePhase(uint16_t deg);

//-----------------------------------------------------------------------------
// main
//-----------------------------------------------------------------------------
void main(void)
{
    // Clocks (SYSCLK = 100 MHz), watchdog off, peripheral clocks on
    Device_init();
    Device_initGPIO();
    Interrupt_initModule();
    Interrupt_initVectorTable();

    initEPWMPins();

    // Stop ALL ePWM time-base clocks while configuring, so that both
    // counters start counting at exactly the same instant later.
    SysCtl_disablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);

    SysCtl_setEPWMClockDivider(SYSCTL_EPWMCLK_DIV_2);   // EPWMCLK = 50 MHz

    // Same frequency / duty / action settings for both modules
    initEPWMCommon(EPWM1_BASE);
    initEPWMCommon(EPWM2_BASE);

    // ---- MASTER: ePWM1 ------------------------------------------------------
    // Does not accept a sync input; sends a SYNCOUT pulse every time its
    // counter reaches zero (start of every PWM period).
    EPWM_disablePhaseShiftLoad(EPWM1_BASE);
    EPWM_setSyncOutPulseMode(EPWM1_BASE, EPWM_SYNC_OUT_PULSE_ON_COUNTER_ZERO);

    // ---- SLAVE: ePWM2 -------------------------------------------------------
    // On every sync pulse from ePWM1, the counter is forced to TBPHS.
    // That forced jump is what creates the fixed phase offset.
    EPWM_enablePhaseShiftLoad(EPWM2_BASE);
    // Pass the sync pulse on down the chain (ePWM2 -> ePWM3) for later use
    EPWM_setSyncOutPulseMode(EPWM2_BASE, EPWM_SYNC_OUT_PULSE_ON_EPWMxSYNCIN);
    setSlavePhase(phaseDeg);

    // Release all time-base clocks together -> counters start in step
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);

    EINT;   // global interrupt enable (not used yet, standard template)
    ERTM;   // real-time debug enable

    for(;;)
    {
        // Change phaseDeg from the Expressions window -> applied here
        if(phaseDeg != appliedPhaseDeg)
        {
            setSlavePhase(phaseDeg);
        }
    }
}

//-----------------------------------------------------------------------------
// Pin mux: route ePWM outputs to the GPIO pins
//-----------------------------------------------------------------------------
static void initEPWMPins(void)
{
    GPIO_setPadConfig(0U, GPIO_PIN_TYPE_STD);
    GPIO_setPinConfig(GPIO_0_EPWM1_A);

    GPIO_setPadConfig(2U, GPIO_PIN_TYPE_STD);
    GPIO_setPinConfig(GPIO_2_EPWM2_A);
}

//-----------------------------------------------------------------------------
// Time-base, counter-compare and action-qualifier setup (same for both)
//-----------------------------------------------------------------------------
static void initEPWMCommon(uint32_t base)
{
    // Keep PWM running when the debugger halts the CPU
    EPWM_setEmulationMode(base, EPWM_EMULATION_FREE_RUN);

    // ---- Time base ----
    EPWM_setClockPrescaler(base, EPWM_CLOCK_DIVIDER_1, EPWM_HSCLOCK_DIVIDER_1);
    EPWM_setTimeBasePeriod(base, EPWM_TBPRD);
    EPWM_setPeriodLoadMode(base, EPWM_PERIOD_SHADOW_LOAD);
    EPWM_setTimeBaseCounter(base, 0U);
    EPWM_setTimeBaseCounterMode(base, EPWM_COUNTER_MODE_UP);

    // ---- Counter compare (duty) ----
    EPWM_setCounterCompareValue(base, EPWM_COUNTER_COMPARE_A, EPWM_DUTY_CMPA);
    EPWM_setCounterCompareShadowLoadMode(base, EPWM_COUNTER_COMPARE_A,
                                         EPWM_COMP_LOAD_ON_CNTR_ZERO);

    // ---- Action qualifier ----
    // Output A goes HIGH at counter = 0, LOW at counter = CMPA (counting up)
    EPWM_setActionQualifierAction(base, EPWM_AQ_OUTPUT_A,
                                  EPWM_AQ_OUTPUT_HIGH,
                                  EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);
    EPWM_setActionQualifierAction(base, EPWM_AQ_OUTPUT_A,
                                  EPWM_AQ_OUTPUT_LOW,
                                  EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);
}

//-----------------------------------------------------------------------------
// Convert a lag in degrees to the TBPHS value for ePWM2.
//
// At each master zero, the slave counter is forced to TBPHS. The slave's own
// zero (its rising edge) therefore happened TBPHS counts EARLIER, i.e. the
// slave LEADS by TBPHS counts. To LAG by N counts:
//     TBPHS = PERIOD - N
// Example 90 deg: N = 5000 * 90 / 360 = 1250 -> TBPHS = 3750
//-----------------------------------------------------------------------------
static void setSlavePhase(uint16_t deg)
{
    uint32_t lagCounts;
    uint16_t tbphs;

    lagCounts = ((uint32_t)EPWM_PERIOD_COUNTS * (uint32_t)(deg % 360U)) / 360U;
    tbphs     = (uint16_t)((EPWM_PERIOD_COUNTS - lagCounts) % EPWM_PERIOD_COUNTS);

    EPWM_setPhaseShift(EPWM2_BASE, tbphs);

    phaseCountsTBPHS = tbphs;
    appliedPhaseDeg  = deg;
}
