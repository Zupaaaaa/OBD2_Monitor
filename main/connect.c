/****************************************************************************
*
* Autor: Filip Kiek
*
* Biblioteka służy do łączenia się z urządzeniem OBD2 poprzez Bluetooth.
*
****************************************************************************/

#include "connect.h"
#include "bluetooth.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "debug.h"

nvs_handle_t memory_handle;

void memory_open(void)
{
    // "storage" to nazwa przestrzeni nazw (namespace), max 15 znaków
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &memory_handle);

    if (err != ESP_OK) {
        ESP_LOGI("Memory", "Błąd (%s) otwierania NVS!", esp_err_to_name(err));
    } else {
        ESP_LOGI("Memory", "NVS otwarte");
    }
}

void memory_save_mac(esp_bd_addr_t mac)
{
    esp_err_t err = nvs_set_blob(memory_handle, "bt_dev_mac", mac, ESP_BD_ADDR_LEN);
    if(err != ESP_OK) {
        ESP_LOGI("MEMORY", "Blad zapisu do pamieci (%s)!", esp_err_to_name(err));
    }
    else {
        ESP_LOGI("MEMORY", "Zapisano nowa wartosc!");
    }

    // Commit
    err = nvs_commit(memory_handle);
    if(err != ESP_OK) {
        ESP_LOGI("MEMORY", "Blad Commit (%s)!", esp_err_to_name(err));
    }
    else {
        ESP_LOGI("MEMORY", "Dane zatwierdzone (Commit).");
    }
}

void memory_close(void)
{
    nvs_close(memory_handle);
    ESP_LOGI("MEMORY", "NVS zamkniete");
}

bool try_connect(esp_bd_addr_t * mac)
{
    app_gap_cb_t *p_dev;
    int device_number;
    char bda_str[18];

    memset(dev_info, 0, sizeof(dev_info)); // wyczysc strukture przechowujaca informacje o urzadzeniach
    bluetooth_scan();

    printf("Wybierz urzadzenie:\n");
    for(device_number = 0; device_number < MAX_DEVICES; device_number++) {
        p_dev = &dev_info[device_number];

        if(p_dev->dev_counter == 0) {
            break;
        }
        
        bda2str(p_dev->bda, bda_str, sizeof(bda_str));
        printf("Urzadzenie %d, MAC %s, Nazwa %s \n", p_dev->dev_counter, bda_str, p_dev->bdname);
        device_number++;
    }
    device_number = wczytajLiczbe();

    memcpy(mac, dev_info[device_number - 1].bda, sizeof(esp_bd_addr_t));
    return bluetooth_connect(*mac);
}

void connect_device(void)
{
    bluetooth_init();
    
    memory_open();

    esp_bd_addr_t mac;

    size_t sizeof_mac = ESP_BD_ADDR_LEN;
    esp_err_t err = nvs_get_blob(memory_handle, "bt_dev_mac", &mac, &sizeof_mac);

    switch (err) {
        case ESP_OK:
            ESP_LOGI("Memory", "Znaleziono MAC w Pamieci.");
            bluetooth_scan();
            for(int device_number = 0; device_number < MAX_DEVICES; device_number++) {
                if(memcmp (mac, dev_info[device_number].bda, ESP_BD_ADDR_LEN) == 0) {
                    ESP_LOGI("BT", "Znaleziono zapisane urzadzenie. Proba polaczenia...");
                    if(bluetooth_connect(mac)) {
                        ESP_LOGI("BT", "Polaczono z zapisanym urzadzeniem\n");

                        return;
                    } else {
                         ESP_LOGI("BT", "Nie udalo sie polaczyc z zapisanym urzadzeniem");
                    }
                }
                else {
                    ESP_LOGI("BT", "Nie znaleziono zapisanego urzadzenia.");
                }
            }
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGI("MEMORY", "Wartość nie zainicjalizowana!");
            break;
        default :
            ESP_LOGI("MEMORY", "Błąd odczytu (%s)", esp_err_to_name(err));
    }
    while(!try_connect(&mac));

    memory_save_mac(mac);
    memory_close();
}
