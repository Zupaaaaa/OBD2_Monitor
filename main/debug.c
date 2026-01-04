#include <stdio.h>
#include <string.h>
#include "debug.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Funkcja blokująca - czeka na wpisanie liczby w monitorze
int wczytajLiczbe(void) {
    char bufor[64]; // Bufor na tekst wpisany przez użytkownika
    int liczba;

    while (1) {
        // 2. Czekaj na całą linię tekstu (blokuje zadanie do momentu wciśnięcia Enter)
        // fgets czyta ze standardowego wejścia (stdin), które w ESP-IDF jest podpięte pod UART
        if (fgets(bufor, sizeof(bufor), stdin) != NULL) {
            
            // 3. Próbujemy przekonwertować tekst na liczbę (sscanf zwraca liczbę sukcesów)
            if (sscanf(bufor, "%d", &liczba) == 1) {
                return liczba; // Udało się, zwracamy liczbę
            } else {
                // Użytkownik wpisał coś, co nie jest liczbą (np. "abc")
                printf("Blad: To nie jest liczba. Sprobuj ponownie.\n");
            }
        }
        
        // Krótkie opóźnienie dla bezpieczeństwa (watchdog), choć fgets zazwyczaj dobrze blokuje
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
