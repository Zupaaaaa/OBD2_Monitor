/****************************************************************************
*
* Autor: Filip Kiek
*
* Biblioteka służy do komunikacji z ukladem ELM327
*
****************************************************************************/

#pragma once

#include <stdint.h>

void elm327_init(void);
uint16_t elm327_get_rpm(void);
uint16_t elm327_get_speed(void);
uint8_t elm327_get_coolant_temp(void);
float elm327_get_maf(void);
float elm327_get_current_fuel_per_hour(float);
float elm327_get_current_fuel_per_100km(float, uint16_t);
float elm327_get_fuel_level(void);