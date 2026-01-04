/****************************************************************************
*
* Autor: Filip Kiek
*
* Biblioteka służy do komunikacji z ukladem ELM327
*
****************************************************************************/

#include "bluetooth.h"
#include "connect.h"

#include "debug.h"

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

void elm327_init(void)
{
    connect_device();
    
    //reset ELM327
    spp_send(CMD_RESET);
    wczytajLiczbe();
    spp_send(CMD_ECHO_OFF);
    wczytajLiczbe();
    spp_send(CMD_LINEFEEDS_OFF);
    wczytajLiczbe();
    spp_send(CMD_SPACES_OFF);
    wczytajLiczbe();
    spp_send(CMD_HEADERS_OFF);
    wczytajLiczbe();
    spp_send(CMD_PROTOCOL_AUTO);
    wczytajLiczbe();
    spp_send(CMD_TEST);
    while(1) {
        wczytajLiczbe();
        spp_send(CMD_ENGINE_RPM);
    }
}
