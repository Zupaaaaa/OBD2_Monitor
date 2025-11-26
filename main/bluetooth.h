/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */



/****************************************************************************
*
* Kod pochodzi z przykładu Espressif Systems "ESP-IDF BT-DISCOVERY Example".
*
* Zmodyfikowany przez: Filip Kiek
*
* Biblioteka służy do obsługi bluetooth classic
*
****************************************************************************/

#pragma once

#include <stdint.h>
#include "esp_gap_bt_api.h"

#define MAX_DEVICES 5

typedef struct {
    uint8_t dev_counter; // numer wykrytego urzadzenia
    uint8_t bdname_len; // dlugosc nazwy urzadzenia
    uint8_t eir_len; // dlugosc danych Extended Inquiry Response
    uint8_t rssi; // siła sygnału
    uint32_t cod; // klasa urzadzenia
    uint8_t eir[ESP_BT_GAP_EIR_DATA_LEN]; // dane Extended Inquiry Response
    uint8_t bdname[ESP_BT_GAP_MAX_BDNAME_LEN + 1]; // nazwa urzadzenia
    esp_bd_addr_t bda; // adres MAC wykrytego urzadzenia
} app_gap_cb_t;

extern app_gap_cb_t dev_info[MAX_DEVICES]; // struktura przechowujaca informacje o wykrytych urzadzeniach

void bluetooth_init(void);
void bluetooth_scan(void);
void bluetooth_service_discover(uint8_t);
bool bluetooth_connect(uint8_t);
void spp_send(char *data);