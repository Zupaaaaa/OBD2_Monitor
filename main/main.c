#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "connect.h"
#include "elm327.h"
#include "averages.h"

void app_main(void)
{
    uint16_t rpm = 0;
    uint16_t speed = 0;
    uint8_t coolant_temp = 0;
    float maf = 0.0;
    float fuel_per_hour = 0.0;
    float fuel_per_100km = 0.0;
    float fuel_level = 0.0;
    float fuel_in_liters = 0.0;
    float average_fuel = 0.0;
    float average_speed = 0.0;
    float estimated_range = 0.0;

    connect_device();
    elm327_init();

    while(1) {
        rpm = elm327_get_rpm();
        speed = elm327_get_speed();
        coolant_temp = elm327_get_coolant_temp();
        maf = elm327_get_maf();
        fuel_per_hour = elm327_get_current_fuel_per_hour(maf);
        fuel_per_100km = elm327_get_current_fuel_per_100km(fuel_per_hour, speed);
        fuel_level = elm327_get_fuel_level();
        fuel_in_liters = calculate_fuel_in_liters(fuel_level);
        average_fuel = get_average_fuel_200km();
        average_speed = get_average_speed_kmh_200km();
        estimated_range = get_estimated_range_km(fuel_in_liters, average_fuel);

        printf("\033[2J\033[H"); // Wyczyść ekran i ustaw kursor w lewym górnym rogu
        printf("RPM: %u\n", rpm);
        printf("Speed: %u km/h\n", speed);
        printf("Coolant Temp: %u C\n", coolant_temp);
        printf("Fuel Consumption: %.1f L/100km / %.1f L/h\n", fuel_per_100km, fuel_per_hour);
        printf("Fuel Level: %.0f %% / %.1f L\n", fuel_level, fuel_in_liters);
        printf("Average Speed: %.1f km/h\n", average_speed);
        printf("Average Fuel Consumption: %.1f L/100km\n", average_fuel);
        printf("Estimated Range: %.1f km\n", estimated_range);

        process_trip_data(maf, speed);
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}
