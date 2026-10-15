#include "driverlib.h"
#include "device.h"
#include "board.h"
#include "c2000_freertos.h"
 
//
// Main
//
void main(void)
{
    Device_init();
    Interrupt_initModule();
    Device_initGPIO();
    DINT;
    IER = 0x0000;
    IFR = 0x0000;
    Interrupt_initVectorTable();
    Board_init();
    FreeRTOS_init();
}
 
void LED_RedTask(void * pvparameters)
{
    (void)pvparameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while(1)
    {
        GPIO_togglePin(myBoardLED1_GPIO);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(500));
    }
}
 
void LED_GreenTask(void * pvparameters)
{
    (void)pvparameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while(1)
    {
        GPIO_togglePin(myBoardLED0_GPIO);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(50));
    }
}
