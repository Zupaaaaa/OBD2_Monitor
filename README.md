# ESP32 OBD-II Monitor

An ESP32-based project that connects to a vehicle's OBD-II port via a Bluetooth adapter (ELM327). It monitors real-time engine parameters and calculates trip statistics that are not natively provided by the engine control unit (ECU).

This project was developed as part of the *Microcontroller Applications* course at **AGH University of Science and Technology** (Microelectronics in Technology and Medicine).

## 🎯 Project Objective
A primary goal of the project was to design an **easy and automatic connection system** with a wireless OBD-II adapter. Additionally, since standard OBD-II protocols do not provide direct readouts for fuel consumption (L/100km) or estimated range, the project includes mathematical models to calculate these values directly on the device using raw sensor data (Mass Air Flow, Vehicle Speed, Fuel Level).

## 🛠 Specification

* **Microcontroller:** ESP32
* **Hardware Interface:** V-Gate iCar II (ELM327-based OBD-II Bluetooth adapter)
* **Language:** C
* **Framework:** ESP-IDF
* **RTOS:** FreeRTOS
* **Communication:** Bluetooth Classic (SPP) & OBD-II

## 💡 Key Implementations

* **Easy and automatic connection system:** Designed for simplicity and "plug-and-play" operation. The ESP32 automatically scans for the saved adapter's MAC address and connects to it. If the known device is unavailable, it gracefully falls back to a discovery mode, allowing the user to easily select a new device.
* **FreeRTOS Integration:** Developed using the native ESP-IDF framework, which uses FreeRTOS to maintain a stable Bluetooth connection in the background while the main application processes vehicle data.
* **Rolling Average (Circular Buffer):** Implemented to track vehicle speed and fuel consumption over a sliding window of the last 200 kilometers.
* **Numerical Integration:** Calculates traveled distance and consumed fuel based on time deltas and data readings.

## 📊 Features

* **Live Telemetry:** Engine RPM, Vehicle Speed, Coolant Temperature, MAF sensor data, Fuel Level.
* **Calculated Parameters:**
  * Instantaneous Fuel Consumption (L/h and L/100km).
  * Long-term Average Fuel Consumption (last 200 km).
  * Estimated Remaining Range (based on fuel level and average consumption).

## 📂 System Flow

1. **Boot:** Initializes system.
2. **Connection:** Executes the automated Bluetooth connecting process with the ELM327 adapter.
3. **Telemetry Loop:** Continuously requests standard PIDs, parses HEX responses, and calculates the rest of parameters.
4. **Trip Statistics:** Processes the incoming data to update the current trip averages.

---
*Developed by Filip Kiek* | *AGH UST WEAIiIB MTM*
