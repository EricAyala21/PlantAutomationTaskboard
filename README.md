# Automatic Plant and web Controller


## Description

An end-to-end IoT ecosystem designed to automate plant care using an **ESP32** microcontroller and a **React-based Web Controller**. This system bridges the gap between hardware sensors and a digital interface, allowing for autonomous plant maintenance or manual remote control via a retro-styled dashboard.

### Core Functions:
* **Real-time Monitoring:** Streams soil moisture and ambient light levels to the web.
* **Manual Overrides:** Trigger the water pump or grow lights instantly from your browser.
* **Autonomous Mode:** (Work in Progress) Decision-tree logic to water plants and toggle lights based on sensor thresholds.
* **Retro UI:** A custom-styled React dashboard featuring CRT monitor effects and SVG animations.


## Getting Started

### Dependencies

### Hardware / Firmware
* **Board:** ESP32 (DevKit V1)
* **Libraries:**
    * `WiFi.h` & `WiFiClientSecure.h`
    * `PubSubClient` (for MQTT)
    * `Arduino.h`

### Software / Web
* **Framework:** React.js
* **Styling:** CSS Animations (CRT flicker/scanlines)
* **MQTT Client:** Browser-based MQTT library

---



### 1. Hardware Setup
1.  Open the source code in the Arduino IDE.
2.  Configure your credentials:
    ```cpp
    const char* ssid = "YOUR_WIFI_NAME";
    const char* password = "YOUR_WIFI_PASSWORD";
    const char* mqtt_broker = "YOUR_MQTT_BROKER_URL";
    ```
3.  Connect your components:
    * **Water Pump:** Pin 22
    * **Light Relay:** Pin 2
    * **Moisture Sensors:** Pins 35, 32
    * **Light Sensor:** Pin 33
4.  Flash the code to your ESP32.

### 2. Web Controller Setup
1.  Navigate to the project folder.
2.  Install the required packages:
    ```bash
    npm install
    ```
3.  Start the development server:
    ```bash
    npm start
    ```

---



## Authors

Contributors names and contact info

 Eric Ayala
 eyala303@gmail.com





## Acknowledgments


*[Science Buddies](https://www.youtube.com/watch?v=ojhrVsBs0nM)
*[Dovid Edelkopf](https://medium.com/@dovid11564/using-css-animations-to-mimic-the-look-of-a-crt-monitor-3919de3318e2)
*[Alec Lownes](https://aleclownes.com/2017/02/01/crt-display.html)
