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

#include "bluetooth.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"

#define CHECK_CONN_DELAY_MS 50
#define MAX_CONN_TIME_MS 2000

app_gap_cb_t dev_info[MAX_DEVICES]; // struktura przechowujaca informacje o wykrytych urzadzeniach

bool scan_complete = false;
bool spp_connected = false;
uint32_t spp_handle = 0;

char *bda2str(esp_bd_addr_t bda, char *str, size_t size)
{
    // sprawdzenie poprawnosci wskaznikow i rozmiaru bufora
    if (bda == NULL || str == NULL || size < 18) {
        return "";
    }

    // konwersja adresu MAC na format string
    snprintf(str, size, "%02x:%02x:%02x:%02x:%02x:%02x",
             bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
    return str;
}

char *uuid2str(esp_bt_uuid_t *uuid, char *str, size_t size)
{
    // sprawdzenie poprawnosci wskaznikow
    if (uuid == NULL || str == NULL) {
        return "";
    }

    // konwersja UUID na format string
    if (uuid->len == 2 && size >= 5) {
        snprintf(str, size, "%04x", uuid->uuid.uuid16);
    } else if (uuid->len == 4 && size >= 9) {
        snprintf(str, size, "%08"PRIx32, uuid->uuid.uuid32);
    } else if (uuid->len == 16 && size >= 37) {
        uint8_t *p = uuid->uuid.uuid128;
        snprintf(str, size, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                 p[15], p[14], p[13], p[12], p[11], p[10], p[9], p[8],
                 p[7], p[6], p[5], p[4], p[3], p[2], p[1], p[0]);
    } else {
        return "";
    }

    return str;
}

bool get_name_from_eir(uint8_t *eir, uint8_t *bdname, uint8_t *bdname_len)
{
    // Inicjalizacja zmiennych pomocniczych (tymczasowych)
    uint8_t *tmp_bdname = NULL;
    uint8_t tmp_bdname_len = 0;

    // Sprawdzenie czy dane EIR istnieja
    if (!eir) {
        return false;
    }

    // Pobranie nazwy urzadzenia z danych EIR
    tmp_bdname = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME, &tmp_bdname_len);

    // Jesli nazwa nie zostala znaleziona, sprobowac pobrac krotka nazwe
    if (!tmp_bdname) {
        tmp_bdname = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_SHORT_LOCAL_NAME, &tmp_bdname_len);
    }

    // Jesli nazwa zostala znaleziona
    if (tmp_bdname) {
        // Ograniczenie dlugosci nazwy do maksymalnej wartosci
        if (tmp_bdname_len > ESP_BT_GAP_MAX_BDNAME_LEN) {
            tmp_bdname_len = ESP_BT_GAP_MAX_BDNAME_LEN;
        }

        // Skopiowanie nazwy do podanego bufora
        if (bdname) {
            memcpy(bdname, tmp_bdname, tmp_bdname_len);
            bdname[tmp_bdname_len] = '\0';
        }

        // Zwrocenie dlugosci nazwy, jesli podano wskaznik
        if (bdname_len) {
            *bdname_len = tmp_bdname_len;
        }
        return true;
    }

    return false;
}

void update_device_info(esp_bt_gap_cb_param_t *param)
{
    // Inicjalizacja zmiennych pomocniczych
    char bda_str[18];
    uint8_t dev_counter;
    uint32_t cod = 0;
    int32_t rssi = -129; // wartosc nieprawidlowa
    uint8_t *bdname = NULL;
    uint8_t bdname_len = 0;
    uint8_t *eir = NULL;
    uint8_t eir_len = 0;
    esp_bt_gap_dev_prop_t *p;

    // Zapisanie informacji o wykrytym urzadzeniu do zmiennych pomocniczych
    for (int i = 0; i < param->disc_res.num_prop; i++) {
        p = param->disc_res.prop + i;
        switch (p->type) {
        case ESP_BT_GAP_DEV_PROP_COD:
            cod = *(uint32_t *)(p->val);
            break;
        case ESP_BT_GAP_DEV_PROP_RSSI:
            rssi = *(int8_t *)(p->val);
            break;
        case ESP_BT_GAP_DEV_PROP_BDNAME:
            bdname_len = (p->len > ESP_BT_GAP_MAX_BDNAME_LEN) ? ESP_BT_GAP_MAX_BDNAME_LEN :
                          (uint8_t)p->len;
            bdname = (uint8_t *)(p->val);
            break;
        case ESP_BT_GAP_DEV_PROP_EIR: {
            eir_len = p->len;
            eir = (uint8_t *)(p->val);
            break;
        }
        default:
            break;
        }
    }

    // Ustalenie numeru znalezionego urzadzenia
    for(dev_counter = 0; dev_counter < MAX_DEVICES; dev_counter++) {
        if (!dev_info[dev_counter].dev_counter) {
            break;
        }
        // sprawdzenie czy urzadzenie nie zostalo juz zarejestrowane
        if(memcmp (param->disc_res.bda, dev_info[dev_counter].bda, ESP_BD_ADDR_LEN) == 0) {
            return;
        }
    }

    // Pomocniczy wskaznik do struktury przechowujacej informacje o urzadzeniu
    app_gap_cb_t *p_dev = &dev_info[dev_counter];

    // Zapisanie numeru urzadzenia
    p_dev->dev_counter = dev_counter + 1;

    // Zapisanie adresu MAC znalezionego urzadzenia
    memcpy(p_dev->bda, param->disc_res.bda, ESP_BD_ADDR_LEN);

    // Zapisanie pozostalych informacji o urzadzeniu
    p_dev->cod = cod;
    p_dev->rssi = rssi;
    if (bdname_len > 0) {
        memcpy(p_dev->bdname, bdname, bdname_len);
        p_dev->bdname[bdname_len] = '\0';
        p_dev->bdname_len = bdname_len;
    }
    if (eir_len > 0) {
        memcpy(p_dev->eir, eir, eir_len);
        p_dev->eir_len = eir_len;
    }

    // Jesli nazwa urzadzenia nie zostala znaleziona, sprobowac pobrac ja z danych EIR
    if (p_dev->bdname_len == 0) {
        get_name_from_eir(p_dev->eir, p_dev->bdname, &p_dev->bdname_len);
    }

    // Wyswietlenie informacji o znalezionym urzadzeniu
    bda2str(p_dev->bda, bda_str, sizeof(bda_str));
    ESP_LOGI("BT", "Urzadzenie %d, MAC %s, Nazwa %s", p_dev->dev_counter, bda_str, p_dev->bdname);

    // Zakoncz wyszukiwanie, jesli osiagnieto maksymalna liczbe urzadzen
    if(dev_counter >= (MAX_DEVICES - 1)) {
        scan_complete = true;
        ESP_LOGI("BT", "Zakonczono wyszukiwanie ...");
        esp_bt_gap_cancel_discovery();
    }
}

void bt_app_gap_init(void)
{
    // Wyczyszczenie struktury przechowujacej informacje o urzadzeniach
    for(uint8_t dev_counter = 0; dev_counter < MAX_DEVICES; dev_counter++) {
        memset(&dev_info[dev_counter], 0, sizeof(app_gap_cb_t));
    }
}

// Callback GAP
void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    // Inicjalizacja zmiennych pomocniczych
    char uuid_str[37];

    switch (event) {
        // zdarzenie wykrycia urzadzenia
        case ESP_BT_GAP_DISC_RES_EVT: {
            // zapisanie informacji o wykrytym urzadzeniu
            update_device_info(param);
            break;
        }
        // zdarzenie zmiany stanu wykrywania urzadzen
        case ESP_BT_GAP_DISC_STATE_CHANGED_EVT: {
            if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED) {
                scan_complete = true;
                ESP_LOGI("BT", "Skanowanie zatrzymane.");
            } else if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STARTED) {
                ESP_LOGI("BT", "Skanowanie rozpoczete.");
            }
            break;
        }
        // zdarzenie wykrycia uslug urzadzenia
        case ESP_BT_GAP_RMT_SRVCS_EVT: {
            // wyswietlenie informacji o znalezionych uslugach
            if (param->rmt_srvcs.stat == ESP_BT_STATUS_SUCCESS) {
                ESP_LOGI("BT", "Znaleziono uslugi dla %s", param->rmt_srvcs.bda);
                for (int i = 0; i < param->rmt_srvcs.num_uuids; i++) {
                    esp_bt_uuid_t *u = param->rmt_srvcs.uuid_list + i;
                    ESP_LOGI("BT", "  %s", uuid2str(u, uuid_str, 37));
                }
            } else {
                ESP_LOGI("BT", "Nie znaleziono uslug dla %s", param->rmt_srvcs.bda);
            }
            scan_complete = true;
            break;
        }
        default: {
            break;
        }
    }
}

void bluetooth_scan(void)
{
    // Rozpoczecie wykrywania urzadzen
    scan_complete = false;
    esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
    
    // Oczekiwanie na zakonczenie wykrywania urzadzen
    while(!scan_complete) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void bluetooth_service_discover(uint8_t device_number)
{
    // Ustalenie liczby znalezionych urzadzen
    uint8_t dev_counter;
    for(dev_counter = 0; dev_counter <= MAX_DEVICES; dev_counter++) {
        if(dev_info[dev_counter].dev_counter == 0) {
            break;
        }
    }

    // Sprawdzenie poprawnosci numeru urzadzenia
    if(device_number == 0 || device_number > dev_counter) {
        ESP_LOGE("BT", "Nieprawidlowy numer urzadzenia!");
        return;
    }

    // Rozpoczecie wykrywania uslug wybranego urzadzenia
    scan_complete = false;
    ESP_LOGI("BT", "Wyszukiwanie uslug ...");
    esp_bt_gap_get_remote_services(dev_info[device_number - 1].bda);

    // Oczekiwanie na zakonczenie wykrywania usulug
    while(!scan_complete) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

// Callback SPP
void spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    switch (event) {
        // Inicjalizacja SPP
        case ESP_SPP_INIT_EVT: {
            ESP_LOGI("SPP", "Inicjalizacja SPP");
            break;
        }
        // Otworzenie polaczenia SPP
        case ESP_SPP_OPEN_EVT: {
            ESP_LOGI("SPP", "Połączono!");
            spp_handle = param->open.handle;
            spp_connected = true;
            break;
        }
        // Otrzymanie danych SPP
        case ESP_SPP_DATA_IND_EVT: {
            ESP_LOGI("SPP", "Odebrano %d bajtów:", param->data_ind.len);
            printf("%.*s\n", param->data_ind.len, param->data_ind.data);
            break;
        }
        // Zamkniecie polaczenia SPP
        case ESP_SPP_CLOSE_EVT: {
            ESP_LOGI("SPP", "Rozłączono");
            spp_connected = false;
            break;
        }
        default: {
            break;
        }
    }
}

void bluetooth_init(void)
{
    // inicjalizacja NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Uwolnienie pamieci BLE, poniewaz uzywamy tylko klasycznego BT
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    // Inicjalizacja kontrolera BT
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));

    // Uruchomienie kontrolera BT w trybie klasycznego BT
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT));

    // Inicjalizacja i uruchomienie stosu bluetooth (Bluedroid)
    esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bluedroid_init_with_cfg(&bluedroid_cfg));
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    // Wyswietlenie adresu MAC ESP
    char bda_str[18] = {0};
    ESP_LOGI("BT", "ESP adres: %s", bda2str((uint8_t *)esp_bt_dev_get_address(), bda_str, sizeof(bda_str)));

    // Rejestracja callbacka GAP
    esp_bt_gap_register_callback(bt_app_gap_cb);

    // Ustawienie nazwy urzadzenia Bluetooth
    char *dev_name = "ESP_32";
    esp_bt_gap_set_device_name(dev_name);

    // Wlaczenie mozliwosci wykrycia i polaczenia
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);

    // Inicjalizaja informacji o urzadzeniu i statusie
    bt_app_gap_init();

    // Inicjalizacja SPP
    ESP_ERROR_CHECK(esp_spp_register_callback(spp_cb));
    ESP_ERROR_CHECK(esp_spp_init(ESP_SPP_MODE_CB));
}

bool bluetooth_connect(uint8_t device_number)
{
        // Ustalenie liczby znalezionych urzadzen
        uint8_t dev_counter;
        for(dev_counter = 0; dev_counter <= MAX_DEVICES; dev_counter++) {
            if(dev_info[dev_counter].dev_counter == 0) {
                break;
            }
        }
    
        // Sprawdzenie poprawnosci numeru urzadzenia
        if(device_number == 0 || device_number > dev_counter) {
            ESP_LOGE("BT", "Nieprawidlowy numer urzadzenia!");
            return false;
        }

        const char *pin[] = {"1234", "0000"};
        for (int i = 0; i < 2; i++) {
            esp_bt_gap_set_pin(ESP_BT_PIN_TYPE_FIXED, 4, (uint8_t*)pin[i]);
            esp_spp_connect(ESP_SPP_SEC_NONE, ESP_SPP_ROLE_MASTER, 0, dev_info[device_number - 1].bda);

            int counter = 0;
            while (!spp_connected && (counter < MAX_CONN_TIME_MS / CHECK_CONN_DELAY_MS)) {
                vTaskDelay(CHECK_CONN_DELAY_MS / portTICK_PERIOD_MS);
                counter += 1;
            }

            if(spp_connected) {
                ESP_LOGI("SPP", "Polaczono z uzyciem pinu: %s", pin[i]);
                return true;
            }
        }
        ESP_LOGI("SPP", "Zaden pin nie jest poprawny, polaczenie nie powiodlo sie");
        return false;
}

void spp_send(char *data)
{
    esp_spp_write(spp_handle, strlen(data), (uint8_t *)data);
}
