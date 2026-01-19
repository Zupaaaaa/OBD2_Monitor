#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "connect.h"
#include "elm327.h"
#include "averages.h"

void app_main(void)
{
    connect_device();
    elm327_init();

    while(1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
