#include "driverlib.h"
#include "device.h"

#define PASS 0U
#define FAIL 1U

uint32_t result = FAIL;
bool wakeISRFlag;
__interrupt void wakeupISR(void);
void main(void)
{
    uint16_t failCount;
    Device_init();
    Interrupt_initModule();
    Interrupt_initVectorTable();
    Interrupt_register(INT_WAKE, &wakeupISR);
    wakeISRFlag = false;
    failCount = 0U;
    SysCtl_setWatchdogMode(SYSCTL_WD_MODE_INTERRUPT);
    Interrupt_enable(INT_WAKE);
    EINT;
    ERTM;
    EALLOW;
    SysCtl_enableWatchdogReset();
    SysCtl_resetWatchdog();
    EDIS;
    SysCtl_setWatchdogWindowValue(0x20U);
    SysCtl_enableWatchdog();
    DEVICE_DELAY_US(15000U);
    if(wakeISRFlag != true)
    {
        failCount++;
    }
    wakeISRFlag = false;
    EALLOW;
    SysCtl_enableWatchdogReset();
    SysCtl_resetWatchdog();
    EDIS;
    EALLOW;
    SysCtl_enableWatchdogReset();
    SysCtl_resetWatchdog();
    EDIS;
    DEVICE_DELAY_US(1U);
    if(wakeISRFlag == true)
    {
        failCount++;
    }
    if(failCount != 0U)
    {
        result = FAIL;
    }
    else
    {
        result = PASS;
    }
    while(1);
}
__interrupt void
wakeupISR(void)
{
    wakeISRFlag = true;
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}
