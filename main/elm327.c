/****************************************************************************
*
* Autor: Filip Kiek
*
* Biblioteka służy do komunikacji z ukladem ELM327
*
****************************************************************************/

#include "elm327.h"
#include "freertos/FreeRTOS.h"
#include "bluetooth.h"
#include "esp_log.h"

#define MAX_INIT_ATTEMPTS 5

#define BUF_SIZE 128

// lista komend ELM327
#define CMD_RESET          "AT Z\r"
#define CMD_ECHO_OFF       "AT E0\r"
#define CMD_LINEFEEDS_OFF  "AT L0\r"
#define CMD_SPACES_OFF     "AT S0\r"
#define CMD_HEADERS_OFF    "AT H0\r"
#define CMD_PROTOCOL_AUTO  "AT SP 0\r"
#define CMD_TEST           "0100\r"
#define CMD_ENGINE_RPM     "010C\r"
#define CMD_VEHICLE_SPEED  "010D\r"

char rx_data[BUF_SIZE];

bool elm327_read_response(char *data) {
    int i;

    if(spp_receive(data)) {
        for(i = 0; data[i] != '>'; i++) {
            // Usuwanie znakow nowej linii i powrotu karetki
            if(data[i] == '\r' || data[i] == '\n') {
                for(int j = i; data[j] != '>'; j++) {
                    data[j] = data[j + 1];
                }
                i--; // cofniecie indeksu, aby sprawdzic nowy znak na tej pozycji
            }
        }
        data[i] = '\0'; // zakoncz string przed znakiem '>'
        ESP_LOGI("ELM327", "Odczytano dane: %s", data);

        return true;
    }
    else {
        ESP_LOGI("ELM327", "Blad odczytu z ELM327");
        return false;
    }
}

bool elm327_initialization(void)
{    
    spp_send(CMD_RESET);

    if(!elm327_read_response(rx_data)) {
        ESP_LOGI("ELM327", "Blad podczas inicjalizacji. Reset ELM327");
        return false;
    }

    spp_send(CMD_ECHO_OFF);

    if(!elm327_read_response(rx_data)) {
        ESP_LOGI("ELM327", "Blad podczas inicjalizacji. Reset ELM327");
        return false;
    }

    spp_send(CMD_LINEFEEDS_OFF);

    if(!elm327_read_response(rx_data)) {
        ESP_LOGI("ELM327", "Blad podczas inicjalizacji. Reset ELM327");
        return false;
    }

    spp_send(CMD_SPACES_OFF);

    if(!elm327_read_response(rx_data)) {
        ESP_LOGI("ELM327", "Blad podczas inicjalizacji. Reset ELM327");
        return false;
    }

    spp_send(CMD_HEADERS_OFF);

    if(!elm327_read_response(rx_data)) {
        ESP_LOGI("ELM327", "Blad podczas inicjalizacji. Reset ELM327");
        return false;
    }

    spp_send(CMD_PROTOCOL_AUTO);

    if(!elm327_read_response(rx_data)) {
        ESP_LOGI("ELM327", "Blad podczas inicjalizacji. Reset ELM327");
        return false;
    }

    spp_send(CMD_TEST);

    if(!elm327_read_response(rx_data)) {
        ESP_LOGI("ELM327", "Blad podczas inicjalizacji. Reset ELM327");
        return false;
    }

    return true;
}

void elm327_read(char *data)
{
    if(!elm327_read_response(data)) {
        ESP_LOGI("ELM327", "Blad podczas odczytu danych. Reset ELM327...");
        elm327_init();
    }
}

void elm327_init(void)
{
    uint8_t counter = 0;
    
    while(!elm327_initialization()) {
        if(counter++ >= MAX_INIT_ATTEMPTS) {
            ESP_LOGI("ELM327", "Nie udalo sie zainicjalizowac ELM327. Reset ESP32");
            esp_restart();
        }
        else {
            ESP_LOGI("ELM327", "Inicjalizacja ELM327...");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}