#include "driverlib.h"
#include "device.h"

// Global variables to monitor
int globalCounter = 0;
int dataArray[5] = {0, 0, 0, 0, 0};

// Function to process data
void processArray(void) {
    int i;
    for(i = 0; i < 5; i++) {
        dataArray[i] = globalCounter * i;
    }
}
int main(void) {
    // Basic system initialization
    Device_init();

    while(1) {
        globalCounter++;

        if(globalCounter > 10) {
            globalCounter = 0; // Reset counter
        }

        processArray();

        // Software delay to simulate processing time
        DEVICE_DELAY_US(100000);
    }
}
