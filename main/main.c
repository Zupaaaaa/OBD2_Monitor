#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bluetooth.h"

void app_main(void)
{
    bluetooth_init();
    bluetooth_scan();

    while(1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
