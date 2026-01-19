/****************************************************************************
*
* Autor: Filip Kiek
*
* Biblioteka służy do obliczania średnich parametrów pojazdu
*
****************************************************************************/

#include "averages.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "esp_log.h"

#define WINDOW_SIZE_KM 200 // Rozmiar okna w km
#define TANK_CAPACITY_LITERS 45.0f

typedef struct {
    // --- PALIWO ---
    float fuel_history[WINDOW_SIZE_KM]; 
    float current_km_fuel_acc;
    float total_window_fuel;

    // --- CZAS ---
    float time_history[WINDOW_SIZE_KM];
    float current_km_time_acc;          
    float total_window_time;            

    // --- DYSTANS ---
    float current_km_dist_acc;          
    float total_window_dist;            
    
    // --- INDEKSY ---
    uint16_t head_index;
    bool buffer_full;
} RollingAvg;

RollingAvg Average = {0}; // Inicjalizacja zerami

void update_rolling_average(float fuel_delta, float dist_delta, float time_delta) {
    
    // Akumulacja bieżąca (Paliwo, Dystans i Czas)
    Average.current_km_fuel_acc += fuel_delta;
    Average.current_km_dist_acc += dist_delta;
    Average.current_km_time_acc += time_delta;

    // Czy zamknięto kilometr?
    if (Average.current_km_dist_acc >= 1.0f) {
        // Jeśli bufor pełny, usuwamy najstarsze dane (Paliwo i Czas)
        if (Average.buffer_full) {
            Average.total_window_fuel -= Average.fuel_history[Average.head_index];
            Average.total_window_time -= Average.time_history[Average.head_index];
        }
        else {
            // Obsługa licznika dystansu okna (rośnie do 200)
            Average.total_window_dist += 1.0f;
            if (Average.total_window_dist >= WINDOW_SIZE_KM) {
                Average.buffer_full = true;
                Average.total_window_dist = (float)WINDOW_SIZE_KM;
            }
        }

        // Zapisujemy nowe dane do historii
        Average.fuel_history[Average.head_index] = Average.current_km_fuel_acc;
        Average.time_history[Average.head_index] = Average.current_km_time_acc;

        // Dodajemy do sumy okna
        Average.total_window_fuel += Average.current_km_fuel_acc;
        Average.total_window_time += Average.current_km_time_acc;

        // Przesunięcie indeksu
        Average.head_index++;
        if (Average.head_index >= WINDOW_SIZE_KM) {
            Average.head_index = 0;
        }

        // Reset akumulatorów (nadmiar dystansu przechodzi dalej)
        Average.current_km_dist_acc -= 1.0f;
        Average.current_km_fuel_acc = 0.0f;
        Average.current_km_time_acc = 0.0f;
    }
}

// Funkcja zwracająca srednie spalanie na 100 km w oknie ostatnich 200 km
float get_average_fuel_200km() {
    // Zabezpieczenie przed dzieleniem przez zero na samym początku
    if (Average.total_window_dist < 1.0f) return 0.0f;

    // Wzór: (Suma Paliwa / Suma Dystansu) * 100
    return (Average.total_window_fuel / Average.total_window_dist) * 100.0f;
}

float get_average_speed_kmh_200km() {
    // Zabezpieczenie przed dzieleniem przez zero
    if (Average.total_window_time < 1.0f) return 0.0f;

    // Wzór: V = s / t
    // s = total_window_dist (km)
    // t = total_window_time (s)
    // Wynik w km/s mnożymy przez 3600, żeby mieć km/h
    
    return (Average.total_window_dist / Average.total_window_time) * 3600.0f;
}

void process_trip_data(float current_maf, uint16_t current_speed) {
    
    static int64_t last_calc_time = 0;

    // Pobierz aktualny czas w mikrosekundach
    int64_t now = esp_timer_get_time();

    // Zabezpieczenie na pierwszy start (żeby nie wyszły głupoty)
    if (last_calc_time == 0) {
        last_calc_time = now;
        return;
    }

    // Oblicz ile czasu minęło od ostatniej pętli (Delta T)
    int64_t dt_us = now - last_calc_time;
    last_calc_time = now; // Zapamiętaj czas na następny raz

    // Zamiana mikrosekund na sekundy (dla wzorów fizycznych)
    float dt_seconds = (float)dt_us / 1000000.0f;

    // --- OBLICZANIE KM DELTA (Przyrost drogi) ---
    // Wzór: Droga [km] = (Prędkość [km/h] * Czas [s]) / 3600
    float dist_delta = ((float)current_speed * dt_seconds) / 3600.0f;


    // --- OBLICZANIE FUEL DELTA (Przyrost paliwa) ---
    // Najpierw MAF (g/s) zamieniamy na Paliwo (L/s)
    // Stałe: AFR=14.7, Gęstość=745 g/L
    // Wzór: L/s = MAF / (14.7 * 745)
    
    float fuel_flow_L_per_sec = 0.0f;
    if (current_maf > 0.1f) {
         fuel_flow_L_per_sec = current_maf / 10951.5f; // 10951.5 to (14.7 * 745)
    }

    // Teraz mnożymy przepływ przez czas trwania pętli
    // Wzór: Litry = (L/s) * s
    float fuel_delta = fuel_flow_L_per_sec * dt_seconds;


    // --- PRZEKAZANIE DO BUFORA KOŁOWEGO ---
    update_rolling_average(fuel_delta, dist_delta, dt_seconds);
}

void save_history_to_nvs() {
    nvs_handle_t my_handle;
    esp_err_t err;

    // Otwórz NVS w trybie zapisu
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "Błąd otwarcia NVS: %s", esp_err_to_name(err));
        return;
    }

    // Zapisz całą strukturę jako BLOB (ciąg bajtów)
    err = nvs_set_blob(my_handle, "trip_data", &Average, sizeof(RollingAvg));

    if (err == ESP_OK) {
        // Zatwierdź zmiany (Commit)
        nvs_commit(my_handle);
        ESP_LOGI("NVS", "Zapisano historię trasy do pamięci!");
    } else {
        ESP_LOGE("NVS", "Błąd zapisu Bloba: %s", esp_err_to_name(err));
    }

    // Zamknij uchwyt
    nvs_close(my_handle);
}

void load_history_from_nvs() {
    nvs_handle_t my_handle;
    esp_err_t err;

    // Otwórz NVS w trybie odczytu
    err = nvs_open("storage", NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGW("NVS", "Pusty NVS lub pierwszy start urządzenia.");
        // Jeśli błąd, po prostu zostawiamy strukturę wyzerowaną (jak przy starcie fabrycznym)
        return;
    }

    // Sprawdź rozmiar danych
    size_t required_size = sizeof(RollingAvg);
    
    // Pobierz dane
    err = nvs_get_blob(my_handle, "trip_data", &Average, &required_size);

    if (err == ESP_OK) {
        ESP_LOGI("NVS", "Pomyślnie wczytano historię trasy!");
    } else {
        ESP_LOGW("NVS", "Nie znaleziono danych trasy.");
    }

    nvs_close(my_handle);
}

float calculate_fuel_in_liters(float percentage) {
    // Zmienna globalna do "wygładzania" odczytu (średnia krocząca)
    static float smoothed_fuel_liters = -1.0f; 

    // Zamiana % na Litry
    float current_liters = (percentage / 100.0f) * TANK_CAPACITY_LITERS;
    
    // Pierwszy odczyt - Zainicjuj zmienną.
    if (smoothed_fuel_liters < 0.0f) {
        smoothed_fuel_liters = current_liters;
    } else {
        // Średnia ważona: 98% starej wartości, 2% nowej.
        smoothed_fuel_liters = (smoothed_fuel_liters * 0.98f) + (current_liters * 0.02f);
    }

    return smoothed_fuel_liters;
}

float get_estimated_range_km(float liters_in_tank, float avg_consumption) {
    // Zabezpieczenie przed dzieleniem przez zero
    if (avg_consumption < 0.1f) {
        return 0.0f;
    }

    // Zabezpieczenie rezerwy 3 litry
    float usable_liters = liters_in_tank - 3.0f;
    if (usable_liters < 0) {
        usable_liters = 0;
    }

    // Wzór: (Litry / Spalanie) * 100
    return (usable_liters / avg_consumption) * 100.0f;
}
