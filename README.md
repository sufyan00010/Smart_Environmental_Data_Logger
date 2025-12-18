# Smart Environmental Data Logger
### ESP32-C3 IoT Project with ESP RainMaker Cloud Integration

---

## 📋 Table of Contents
- [Project Overview](#project-overview)
- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
- [Software Requirements](#software-requirements)
- [System Architecture](#system-architecture)
- [Installation & Setup](#installation--setup)
- [Configuration](#configuration)
- [Usage Guide](#usage-guide)
- [Code Structure](#code-structure)
- [RTOS Task Description](#rtos-task-description)
- [RainMaker Integration](#rainmaker-integration)
- [Voice Assistant Setup](#voice-assistant-setup)
- [OTA Updates](#ota-updates)
- [ESP Insights Dashboard](#esp-insights-dashboard)
- [Troubleshooting](#troubleshooting)
- [Future Enhancements](#future-enhancements)
- [License](#license)

---

## 🎯 Project Overview

The **Smart Environmental Data Logger** is an advanced IoT system that continuously monitors environmental conditions (temperature, humidity, and air quality) and provides real-time alerts when thresholds are exceeded. The system integrates with Espressif's RainMaker cloud platform for remote monitoring and control via mobile applications and voice assistants.

### Key Highlights
- ✅ Real-time environmental monitoring
- ✅ Push notifications for critical conditions
- ✅ Cloud-based remote access via RainMaker mobile app
- ✅ Voice control (Google Assistant / Amazon Alexa)
- ✅ OTA firmware updates
- ✅ ESP Insights dashboard analytics
- ✅ FreeRTOS-based multitasking architecture

---

## ✨ Features

### Core Features
1. **Multi-Sensor Monitoring**
   - DHT11: Temperature & Humidity
   - LDR: Light level (used for AQI calculation)
   - Calculated Air Quality Index (AQI)

2. **Real-Time Alerts**
   - Visual indicators (Green/Red LEDs)
   - Audio alerts (Buzzer)
   - Push notifications to mobile app

3. **Cloud Integration**
   - ESP RainMaker for remote monitoring
   - Real-time parameter updates
   - Configurable thresholds from mobile app

4. **Local Display**
   - OLED screen showing live sensor readings
   - Alert status indication

5. **Voice Assistant Integration**
   - Google Assistant support
   - Amazon Alexa support
   - Voice commands for status checks and control

6. **OTA Firmware Updates**
   - Remote firmware upgrades
   - Version tracking
   - Rollback support

7. **Analytics Dashboard**
   - ESP Insights integration
   - Device health monitoring
   - Custom metrics tracking

---

## 🛠️ Hardware Requirements

### Bill of Materials (BOM)

| Component | Quantity | Purpose | GPIO Connection |
|-----------|----------|---------|-----------------|
| ESP32-C3 DevKit | 1 | Main microcontroller | - |
| DHT11 Sensor | 1 | Temperature & Humidity | GPIO4 |
| LDR (Photo Resistor) | 1 | Light sensing (AQI proxy) | GPIO0 (ADC) |
| 10kΩ Resistors | 3 | Pullup/divider circuits | - |
| OLED Display (SSD1306) | 1 | Local status display | GPIO8 (SDA), GPIO9 (SCL) |
| LED (Green) | 1 | Normal status indicator | GPIO2 |
| LED (Red) | 1 | Alert status indicator | GPIO3 |
| 220Ω Resistors | 2 | LED current limiting | - |
| Active Buzzer | 1 | Audio alerts | GPIO10 |
| Push Button | 1 | Manual control/calibration | GPIO5 |
| Breadboard | 1 | Prototyping | - |
| Jumper Wires | ~20 | Connections | - |
| USB-C Cable | 1 | Power & Programming | - |

### Circuit Diagram

```
ESP32-C3 DevKit
│
├─ GPIO4 ──────┬── DHT11 (Data) ─── VCC (3.3V)
│              └── 10kΩ ─── VCC (3.3V)  [Pullup]
│
├─ GPIO0 (ADC) ── LDR ── GND
│              └── 10kΩ ── VCC (3.3V)
│
├─ GPIO8 (SDA) ── OLED SDA
├─ GPIO9 (SCL) ── OLED SCL
│
├─ GPIO2 ─── 220Ω ─── Green LED ─── GND
├─ GPIO3 ─── 220Ω ─── Red LED ─── GND
│
├─ GPIO10 ─── Buzzer (+) ─── GND
│
└─ GPIO5 ─── Push Button ─── GND
           └── 10kΩ ─── VCC (3.3V)  [Pullup]
```

---

## 💻 Software Requirements

### Development Environment
- **ESP-IDF**: v5.2.2 or later
- **Espressif IDE**: v3.0.0 or later (OR VS Code with ESP-IDF extension)
- **Python**: 3.8+ (for ESP-IDF tools)
- **Git**: For version control

### Required SDKs & Libraries
- ESP RainMaker SDK (GitHub: espressif/esp-rainmaker)
- ESP Insights SDK (included in RainMaker)
- DHT11 driver (custom component)
- SSD1306 OLED driver (custom component)

### Mobile Application
- **ESP RainMaker App** (Android/iOS)
  - [Download for Android](https://play.google.com/store/apps/details?id=com.espressif.rainmaker)
  - [Download for iOS](https://apps.apple.com/app/esp-rainmaker/id1497491540)

---

## 🏗️ System Architecture

### High-Level Block Diagram

```
┌─────────────────────────────────────────────────────┐
│                  ESP32-C3 Device                     │
│  ┌───────────┐  ┌───────────┐  ┌─────────────┐     │
│  │  Sensors  │  │   OLED    │  │   Alerts    │     │
│  │ DHT11+LDR │  │  Display  │  │ LEDs+Buzzer │     │
│  └─────┬─────┘  └─────┬─────┘  └──────┬──────┘     │
│        │              │                │            │
│  ┌─────▼──────────────▼────────────────▼──────┐    │
│  │         FreeRTOS Task Management           │    │
│  │  ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐ │    │
│  │  │Sens.│ │Cloud│ │Disp.│ │Alert│ │ OTA │ │    │
│  │  │Task │ │Task │ │Task │ │Task │ │Task │ │    │
│  │  └──┬──┘ └──┬──┘ └──┬──┘ └──┬──┘ └──┬──┘ │    │
│  └─────┼───────┼───────┼───────┼───────┼─────┘    │
│        │       │       │       │       │          │
│  ┌─────▼───────▼───────▼───────▼───────▼─────┐    │
│  │    Queue / Semaphore / Event Groups        │    │
│  └─────────────────┬───────────────────────────┘    │
│                    │                                │
│              ┌─────▼─────┐                          │
│              │ Wi-Fi +   │                          │
│              │ RainMaker │                          │
│              └─────┬─────┘                          │
└────────────────────┼──────────────────────────────┘
                     │
            ┌────────▼────────┐
            │   Internet/     │
            │ RainMaker Cloud │
            └────────┬────────┘
                     │
       ┌─────────────┼─────────────┐
       │             │             │
┌──────▼──────┐ ┌───▼────┐ ┌─────▼──────┐
│ Mobile App  │ │ Google │ │  Insights  │
│  Control    │ │  Home  │ │ Dashboard  │
└─────────────┘ └────────┘ └────────────┘
```

### Data Flow Diagram

```
Sensor Task (10s interval)
    │ Read DHT11 + LDR
    ├─► Calculate AQI
    └─► Send to Queue ───────────┐
                                 │
                       ┌─────────▼─────────┐
                       │  Sensor Data      │
                       │      Queue        │
                       └─────────┬─────────┘
                                 │
              ┌──────────────────┼──────────────────┐
              │                  │                  │
        ┌─────▼──────┐   ┌──────▼──────┐   ┌──────▼──────┐
        │ Cloud Task │   │ Display Task│   │ Alert Task  │
        │  (Receive) │   │  (Receive)  │   │   (Peek)    │
        └─────┬──────┘   └──────┬──────┘   └──────┬──────┘
              │                  │                  │
        ┌─────▼──────┐   ┌──────▼──────┐   ┌──────▼──────┐
        │ RainMaker  │   │ OLED Update │   │ Threshold   │
        │  Update    │   │             │   │   Check     │
        └────────────┘   └─────────────┘   └──────┬──────┘
                                                   │
                                             ┌─────▼──────┐
                                             │ LED/Buzzer │
                                             │  Control   │
                                             │ Push Notif │
                                             └────────────┘
```

---

## 🚀 Installation & Setup

### Step 1: Clone the Repository

```bash
git clone https://github.com/yourusername/smart-environmental-logger.git
cd smart-environmental-logger
```

### Step 2: Install ESP-IDF

Follow the official guide: [ESP-IDF Getting Started](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/get-started/)

```bash
# Linux/macOS
cd ~/esp
git clone -b v5.2.2 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32c3
source export.sh

# Windows (Command Prompt)
cd C:\esp
git clone -b v5.2.2 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
install.bat esp32c3
export.bat
```

### Step 3: Install ESP RainMaker SDK

```bash
cd ~/esp  # or C:\esp on Windows
git clone --recursive https://github.com/espressif/esp-rainmaker.git
```

### Step 4: Update CMakeLists.txt Path

Edit `CMakeLists.txt` in project root:

```cmake
set(RMAKER_PATH "C:/esp/esp-rainmaker")  # Update to your path
```

### Step 5: Build the Project

```bash
cd smart-environmental-logger
idf.py set-target esp32c3
idf.py menuconfig  # Optional: Configure project settings
idf.py build
```

### Step 6: Flash to ESP32-C3

```bash
idf.py -p COM3 flash monitor  # Replace COM3 with your port
```

---

## ⚙️ Configuration

### Wi-Fi Provisioning

On first boot, the device will start in provisioning mode:

1. Open **ESP RainMaker App**
2. Tap **"Add Device"**
3. Scan the QR code shown on serial monitor
4. Follow app instructions to connect to Wi-Fi

### Alert Thresholds (Configurable via App)

Default values (can be changed in RainMaker app):

```c
Temperature High: 35.0°C
Temperature Low:  15.0°C
Humidity High:    80.0%
Humidity Low:     30.0%
AQI Threshold:    150
Buzzer Enabled:   true
```

---

## 📱 Usage Guide

### Mobile App Control

1. **View Live Data**: See real-time temperature, humidity, and AQI
2. **Adjust Thresholds**: Use sliders to set alert limits
3. **Enable/Disable Buzzer**: Toggle audio alerts
4. **View Alert History**: Check past notifications

### Local Status Indicators

- **Green LED ON**: All conditions normal
- **Red LED ON**: Alert condition detected
- **Buzzer Beeping**: Active alert (if enabled)
- **OLED Display**: Shows current readings

### Button Controls

- **Short Press**: Force sensor reading update
- **Long Press (3s)**: Enter calibration mode

---

## 📂 Code Structure

```
smart_environmental_logger/
├── main/
│   ├── app_main.c           # Main application & RainMaker setup
│   ├── sensor_task.c        # Sensor reading task
│   ├── cloud_task.c         # Cloud communication task
│   ├── display_task.c       # OLED display task (to implement)
│   ├── alert_task.c         # Alert monitoring & notifications
│   ├── ota_task.c           # OTA update handler (to implement)
│   ├── app_driver.c         # Hardware initialization
│   └── CMakeLists.txt
├── components/
│   ├── dht11/               # DHT11 driver
│   │   ├── dht11.c
│   │   ├── dht11.h
│   │   └── CMakeLists.txt
│   └── ssd1306/             # OLED driver
│       ├── ssd1306.c
│       ├── ssd1306.h
│       └── CMakeLists.txt
├── CMakeLists.txt           # Root build configuration
├── sdkconfig                # ESP-IDF configuration
├── partitions.csv           # Custom partition table (for OTA)
└── README.md                # This file
```

---

## 🧵 RTOS Task Description

### Task Hierarchy & Priorities

| Task Name | Priority | Core | Stack Size | Period | Purpose |
|-----------|----------|------|------------|--------|---------|
| Sensor Task | 5 | 1 | 4096 | 10s | Read DHT11, LDR, calculate AQI |
| Cloud Task | 4 | 0 | 4096 | Event-driven | Send data to RainMaker |
| Display Task | 3 | 1 | 4096 | 2s | Update OLED screen |
| Alert Task | 6 (Highest) | 1 | 4096 | 2s | Monitor thresholds, trigger alerts |
| OTA Task | 2 (Lowest) | 0 | 4096 | On-demand | Handle firmware updates |

### Inter-Task Communication

```c
// Sensor → Cloud/Display/Alert
QueueHandle_t sensor_data_queue;  // Max 10 items

// Mutual exclusion for RainMaker API
SemaphoreHandle_t rainmaker_mutex;

// System-wide events
EventGroupHandle_t system_events;
  - BIT0: WIFI_CONNECTED
  - BIT1: CLOUD_CONNECTED
  - BIT2: ALERT_TRIGGERED
```

---

## ☁️ RainMaker Integration

### Node Structure

```
Node: "Environmental Logger" (Type: Sensor)
├── Device 1: "Temperature" (Type: Temperature Sensor)
│   ├── Temperature (float, read-only, °C)
│   ├── Temp High Threshold (float, read-write, slider 25-50°C)
│   └── Temp Low Threshold (float, read-write, slider 0-25°C)
├── Device 2: "Humidity" (Type: Temperature Sensor)
│   ├── Humidity (float, read-only, %)
│   ├── Humidity High Threshold (float, read-write, slider 60-100%)
│   └── Humidity Low Threshold (float, read-write, slider 0-40%)
├── Device 3: "Air Quality" (Type: Temperature Sensor)
│   ├── AQI (int, read-only)
│   └── Air Quality Status (string, read-only: Good/Moderate/Unhealthy)
└── Device 4: "Alert System" (Type: Switch)
    ├── Buzzer (bool, read-write, toggle)
    └── Alert Status (string, read-only: push notification text)
```

---

## 🎤 Voice Assistant Setup

### Google Assistant

1. Open **Google Home** app
2. Tap **"+"** → **"Set up Device"** → **"Works with Google"**
3. Search for **"ESP RainMaker"**
4. Sign in with your RainMaker credentials
5. Devices will appear automatically

**Example Commands:**
- "Hey Google, what's the temperature on Environmental Logger?"
- "Hey Google, check the humidity"
- "Hey Google, turn off the buzzer on Environmental Logger"

### Amazon Alexa

Follow the [official guide](https://rainmaker.espressif.com/docs/alexa.html)

---

## 🔄 OTA Updates

### Triggering OTA from RainMaker Dashboard

1. Log in to [RainMaker Dashboard](https://dashboard.rainmaker.espressif.com/)
2. Go to **"Firmware Images"** → **"Add Image"**
3. Upload your `.bin` file (found in `build/` folder)
4. Go to **"OTA Jobs"** → **"Start OTA"**
5. Select target device and confirm

### Firmware Version Tracking

Defined in `CMakeLists.txt`:
```cmake
set(PROJECT_VER "1.0.0")
```

---

## 📊 ESP Insights Dashboard

View at: [insights.dashboard.rainmaker.espressif.com](https://insights.dashboard.rainmaker.espressif.com/)

### Metrics Tracked
- Free heap memory
- Wi-Fi signal strength (RSSI)
- Device uptime
- Reset reasons
- Custom metrics:
  - AQI good/moderate/unhealthy counts
  - Alert frequency
  - Sensor read failures

---

## 🐛 Troubleshooting

### Device Not Connecting to Wi-Fi
- Ensure correct SSID/password
- Check 2.4 GHz Wi-Fi (ESP32-C3 doesn't support 5 GHz)
- Reset provisioning: Hold BOOT button for 5s

### Sensor Readings Incorrect
- Verify wiring (especially DHT11 pullup resistor)
- Check GPIO assignments in code
- Try sensor calibration mode (long press button)

### No Push Notifications
- Verify RainMaker cloud connection (check logs)
- Ensure alert thresholds are set correctly
- Check notification cooldown (1 minute default)

### OTA Fails
- Verify sufficient flash space (`idf.py size`)
- Check partition table (`partitions.csv`)
- Ensure stable Wi-Fi during update

---

## 🚀 Future Enhancements

- [ ] Add BME680 for true VOC/gas sensing
- [ ] Implement data logging to SD card
- [ ] Add web server for local access
- [ ] Machine learning for predictive alerts
- [ ] Battery-powered version with deep sleep
- [ ] Integration with IFTTT/Home Assistant

---

## 📄 License

MIT License - See LICENSE file for details

---

## 👨‍💻 Author

**[SUFYAN JAAFAR]**
- GitHub: [@sufyan00010](https://github.com/sufyan00010)
- Email: sufyanjabubakar@gmail.com

---

## 🙏 Acknowledgments

- Espressif Systems for ESP-IDF and RainMaker
- FreeRTOS community
- Course: IBM 5043 - Advanced Embedded Systems

---

**⭐ If you found this project helpful, please give it a star!**
