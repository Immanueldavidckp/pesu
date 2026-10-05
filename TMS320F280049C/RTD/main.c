#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "driverlib.h"
#include "device.h"
#include "board.h"
 
/* ================= User configuration ================= */
#define RTD_SPI_BASE     mySPI0_BASE
 
#define RTD_NOMINAL      100.0f     /* PT100 = 100, PT1000 = 1000            */
#define RTD_RREF         430.0f     /* Adafruit: 430 (PT100), 4300 (PT1000)  */
#define RTD_3WIRE        true       /* false for 2-wire or 4-wire            */
#define FILTER_50HZ      true       /* India mains = 50 Hz                   */
 
/* ================= MAX31865 definitions ================= */
#define REG_CONFIG       0x00U
#define REG_RTDMSB       0x01U
#define REG_HFAULTMSB    0x03U
#define REG_HFAULTLSB    0x04U
#define REG_LFAULTMSB    0x05U
#define REG_LFAULTLSB    0x06U
#define REG_FAULTSTAT    0x07U
 
#define CFG_BIAS         0x80U
#define CFG_MODEAUTO     0x40U
#define CFG_1SHOT        0x20U
#define CFG_3WIRE        0x10U
#define CFG_FAULTSTAT    0x02U
#define CFG_FILT50HZ     0x01U
 
#define FAULT_HIGHTHRESH 0x80U
#define FAULT_LOWTHRESH  0x40U
#define FAULT_REFINLOW   0x20U
#define FAULT_REFINHIGH  0x10U
#define FAULT_RTDINLOW   0x08U
#define FAULT_OVUV       0x04U
 
#define RTD_A            3.9083e-3f
#define RTD_B           -5.775e-7f
 
#define MAX_XFER_BYTES   3U         /* longest transaction: addr + 2 data */
 
/* ================= Watch in CCS Expressions ================= */
volatile uint16_t g_rtdRaw;
volatile float    g_rtdOhms;
volatile float    g_tempC;
volatile uint16_t g_fault;
 
/* ================= Low-level SPI (PTE as CS) ================= */
/*
 * One complete CS-low transaction. All TX bytes are queued in the FIFO
 * back-to-back so PTE stays asserted, then the same number of RX bytes
 * is collected. TX is left-justified (<< 8) for 8-bit characters;
 * RX is right-justified.
 */
static void spiTransaction(const uint16_t *tx, uint16_t *rx, uint16_t n)
{
    uint16_t i;
    uint16_t intState;
 
    SPI_resetRxFIFO(RTD_SPI_BASE);              /* discard stale bytes */
 
    intState = __disable_interrupts();          /* no gaps between bytes */
    for (i = 0U; i < n; i++)
    {
        SPI_writeDataNonBlocking(RTD_SPI_BASE, (tx[i] & 0xFFU) << 8);
    }
    __restore_interrupts(intState);
 
    while ((uint16_t)SPI_getRxFIFOStatus(RTD_SPI_BASE) < n) { }
 
    for (i = 0U; i < n; i++)
    {
        uint16_t v = SPI_readDataNonBlocking(RTD_SPI_BASE) & 0xFFU;
        if (rx != NULL)
        {
            rx[i] = v;
        }
    }
}
 
static void readRegisterN(uint16_t addr, uint16_t *buf, uint16_t n)
{
    uint16_t tx[MAX_XFER_BYTES];
    uint16_t rx[MAX_XFER_BYTES];
    uint16_t i;
 
    tx[0] = addr & 0x7FU;                       /* read: MSB = 0 */
    for (i = 1U; i <= n; i++)
    {
        tx[i] = 0xFFU;                          /* dummy clocks  */
    }
 
    spiTransaction(tx, rx, n + 1U);
 
    for (i = 0U; i < n; i++)
    {
        buf[i] = rx[i + 1U];                    /* rx[0] is junk during addr */
    }
}
 
static uint16_t readRegister8(uint16_t addr)
{
    uint16_t v = 0U;
    readRegisterN(addr, &v, 1U);
    return v;
}
 
static uint16_t readRegister16(uint16_t addr)
{
    uint16_t b[2] = {0U, 0U};
    readRegisterN(addr, b, 2U);
    return (uint16_t)((b[0] << 8) | b[1]);
}
 
static void writeRegister8(uint16_t addr, uint16_t data)
{
    uint16_t tx[2];
    tx[0] = addr | 0x80U;                       /* write: MSB = 1 */
    tx[1] = data & 0xFFU;
    spiTransaction(tx, NULL, 2U);
}
 
static void setConfigBit(uint16_t mask, bool set)
{
    uint16_t t = readRegister8(REG_CONFIG);
    t = set ? (t | mask) : (t & ~mask);
    writeRegister8(REG_CONFIG, t & 0xFFU);
}
 
/* ================= MAX31865 functions ================= */
static void MAX31865_clearFault(void)
{
    uint16_t t = readRegister8(REG_CONFIG);
    t &= ~0x2CU;
    t |= CFG_FAULTSTAT;
    writeRegister8(REG_CONFIG, t & 0xFFU);
}
 
/* Fault-detection cycle options (same as Adafruit max31865_fault_cycle_t).
 * Adafruit's readFault() defaults to FAULT_CYCLE_AUTO, and the example
 * sketch calls it with no argument, so the main loop uses AUTO. */
typedef enum {
    FAULT_CYCLE_NONE = 0,         /* just read FAULTSTAT                  */
    FAULT_CYCLE_AUTO,             /* run automatic open/short detection   */
    FAULT_CYCLE_MANUAL_RUN,
    FAULT_CYCLE_MANUAL_FINISH
} FaultCycle;
 
static uint16_t MAX31865_readFault(FaultCycle cycle)
{
    if (cycle != FAULT_CYCLE_NONE)
    {
        uint16_t cfg = readRegister8(REG_CONFIG) & 0x11U;  /* keep wire + filter bits */
 
        switch (cycle)
        {
            case FAULT_CYCLE_AUTO:
                writeRegister8(REG_CONFIG, cfg | 0x84U);   /* bias on + auto fault detect */
                DEVICE_DELAY_US(1000U);
                break;
            case FAULT_CYCLE_MANUAL_RUN:
                writeRegister8(REG_CONFIG, cfg | 0x88U);
                return 0U;
            case FAULT_CYCLE_MANUAL_FINISH:
                writeRegister8(REG_CONFIG, cfg | 0x8CU);
                return 0U;
            default:
                break;
        }
    }
    return readRegister8(REG_FAULTSTAT);
}
 
static void MAX31865_setThresholds(uint16_t lower, uint16_t upper)
{
    writeRegister8(REG_LFAULTLSB, lower & 0xFFU);
    writeRegister8(REG_LFAULTMSB, lower >> 8);
    writeRegister8(REG_HFAULTLSB, upper & 0xFFU);
    writeRegister8(REG_HFAULTMSB, upper >> 8);
}
 
static void MAX31865_init(void)
{
    /* Make sure FIFO mode is on with no inter-word delay,
       even if SysConfig was left at defaults */
    SPI_enableFIFO(RTD_SPI_BASE);
    SPI_setTxFifoTransmitDelay(RTD_SPI_BASE, 0U);
    SPI_resetTxFIFO(RTD_SPI_BASE);
    SPI_resetRxFIFO(RTD_SPI_BASE);
 
    setConfigBit(CFG_3WIRE, RTD_3WIRE);
    setConfigBit(CFG_BIAS, false);
    setConfigBit(CFG_MODEAUTO, false);
    setConfigBit(CFG_FILT50HZ, FILTER_50HZ);
    MAX31865_setThresholds(0x0000U, 0xFFFFU);
    MAX31865_clearFault();
}
 
/* One-shot conversion, returns 15-bit raw RTD code (blocks ~75 ms) */
static uint16_t MAX31865_readRTD(void)
{
    uint16_t rtd;
 
    MAX31865_clearFault();
    setConfigBit(CFG_BIAS, true);
    DEVICE_DELAY_US(10000U);                    /* bias settle 10 ms  */
 
    setConfigBit(CFG_1SHOT, true);
    DEVICE_DELAY_US(65000U);                    /* conversion 65 ms   */
 
    rtd = readRegister16(REG_RTDMSB);
    setConfigBit(CFG_BIAS, false);              /* reduce self-heating */
 
    return rtd >> 1;                            /* drop fault bit D0  */
}
 
/* Callendar-Van Dusen; polynomial fit below 0 degC */
static float MAX31865_calcTemp(uint16_t raw, float rNominal, float rRef)
{
    float Z1, Z2, Z3, Z4, Rt, temp, rpoly;
 
    Rt = ((float)raw / 32768.0f) * rRef;
 
    Z1 = -RTD_A;
    Z2 = RTD_A * RTD_A - (4.0f * RTD_B);
    Z3 = (4.0f * RTD_B) / rNominal;
    Z4 = 2.0f * RTD_B;
 
    temp = (sqrtf(Z2 + Z3 * Rt) + Z1) / Z4;
    if (temp >= 0.0f)
    {
        return temp;
    }
 
    Rt = (Rt / rNominal) * 100.0f;              /* normalise to 100 ohm */
    rpoly = Rt;
    temp  = -242.02f;
    temp += 2.2228f     * rpoly;  rpoly *= Rt;
    temp += 2.5859e-3f  * rpoly;  rpoly *= Rt;
    temp -= 4.8260e-6f  * rpoly;  rpoly *= Rt;
    temp -= 2.8183e-8f  * rpoly;  rpoly *= Rt;
    temp += 1.5243e-10f * rpoly;
    return temp;
}
 
/* ================= Main ================= */
void main(void)
{
    Device_init();
    Device_initGPIO();
    Interrupt_initModule();
    Interrupt_initVectorTable();
    Board_init();                               /* SysConfig SPI + pins */
    EINT;
    ERTM;
 
    MAX31865_init();
 
    for (;;)
    {
        g_rtdRaw  = MAX31865_readRTD();
        g_rtdOhms = RTD_RREF * ((float)g_rtdRaw / 32768.0f);
        g_tempC   = MAX31865_calcTemp(g_rtdRaw, RTD_NOMINAL, RTD_RREF);
 
        g_fault = MAX31865_readFault(FAULT_CYCLE_AUTO);   /* same as example -> break point */
        if (g_fault != 0U)
        {
            MAX31865_clearFault();
        }
 
        DEVICE_DELAY_US(500000U);               /* ~2 readings/sec */
    }
}
