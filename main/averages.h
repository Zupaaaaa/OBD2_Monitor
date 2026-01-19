/****************************************************************************
*
* Autor: Filip Kiek
*
* Biblioteka służy do obliczania średnich parametrów pojazdu
*
****************************************************************************/

#pragma once

#include <stdint.h>

float get_average_fuel_200km();
float get_average_speed_kmh_200km();
float calculate_fuel_in_liters(float);
float get_estimated_range_km(float, float);
void process_trip_data(float, uint16_t);