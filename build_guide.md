# Build and Flash Guide - Smart Environmental Logger

## 📋 Prerequisites

### Hardware
- ✅ ESP32-C3 DevKit (Seeed Studio XIAO ESP32-C3 or ESP32-C3-DevKitC-02)
- ✅ USB-C cable (data + power)
- ✅ All components assembled as per circuit diagram

### Software
- ✅ ESP-IDF v5.2.2 installed
- ✅ ESP RainMaker SDK cloned
- ✅ VS Code with ESP-IDF extension (OR Espressif-IDE)
- ✅ Python 3.8+

---

## 🚀 Step-by-Step Build Instructions

### Step 1: Install ESP-IDF v5.2.2

#### Windows
```cmd
cd C:\
mkdir esp
cd esp
git clone -b v5.2.2 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
install.bat esp32c3
```

#### Linux/macOS
```bash
cd ~/
mkdir esp
cd esp
git clone -b v5.2.2 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32c3
```

### Step 2: Install ESP RainMaker SDK

```bash
# Windows
cd C:\esp
git clone --recursive https://github.com/espressif/esp-rainmaker.git

# Linux/macOS
cd ~/esp
git clone --recursive https://github.com/espressif/esp-rainmaker.git
```

### Step 3: Clone This Project

```bash
# Windows
cd C:\Users\YourName\Documents
git clone https://github.com/yourusername/smart-environmental-logger.git
cd smart-environmental-logger

# Linux/macOS
cd ~/Documents
git clone https://github.com/yourusername/smart-environmental-logger.git
cd smart-environmental-logger
```

### Step 4: Configure RainMaker Path

Edit `CMakeLists.txt` (line 7-13):

```cmake
if(WIN32)
    set(RMAKER_PATH "C:/esp/esp-rainmaker")  # ← Update this
else()
    set(RMAKER_PATH "$ENV{HOME}/esp/esp-rainmaker")  # ← Update this
endif()
```

### Step 5: Set ESP-IDF Environment

#### Windows (Command Prompt)
```cmd
cd C:\esp\esp-idf
export.bat
```

#### Linux/macOS (Terminal)
```bash
cd ~/esp/esp-idf
source export.sh
```

**OR** in VS Code:
1. Open Command Palette (Ctrl+Shift+P / Cmd+Shift+P)
2. Type "ESP-IDF: Configure ESP-IDF Extension"
3. Select your ESP-IDF path

---

## 🔧 Building the Project

### Method 1: Using VS Code (Recommended)

1. **Open Project in VS Code:**
   ```bash
   code smart-environmental-logger
   ```

2. **Select Target:**
   - Press `Ctrl+E` then `D` (or click ESP-IDF icon in sidebar)
   - Select "ESP32-C3"

3. **Build:**
   - Press `Ctrl+E` then `B` (or click "Build" in ESP-IDF menu)
   - Wait for build to complete (~2-5 minutes first time)

4. **Flash:**
   - Connect ESP32-C3 via USB
   - Press `Ctrl+E` then `F` (or click "Flash")
   - Select correct COM port (Windows: COM3, Linux: /dev/ttyUSB0)

5. **Monitor:**
   - Press `Ctrl+E` then `M` (or click "Monitor")
   - See serial output

### Method 2: Using Command Line

```bash
cd smart-environmental-logger

# Set target
idf.py set-target esp32c3

# Configure (optional)
idf.py menuconfig

# Build
idf.py build

# Flash
idf.py -p COM3 flash  # Windows
idf.py -p /dev/ttyUSB0 flash  # Linux/macOS

# Monitor
idf.py -p COM3 monitor  # Windows
idf.py -p /dev/ttyUSB0 monitor  # Linux/macOS

# Flash + Monitor (combined)
idf.py -p COM3 flash monitor
```

---

## 📱 Initial Setup (Wi-Fi Provisioning)

### Step 1: Install RainMaker App

- **Android:** [Google Play](https://play.google.com/store/apps/details?id=com.espressif.rainmaker)
- **iOS:** [App Store](https://apps.apple.com/app/esp-rainmaker/id1497491540)

### Step 2: First Boot

1. After flashing, device will start in **provisioning mode**
2. Monitor serial output:
   ```
   I (1234) APP_MAIN: === Smart Environmental Data Logger ===
   I (1235) APP_MAIN: Version: 1.0.0
   I (2345) PROV: Scan the QR code below to provision the device:
   ```

3. A QR code will appear in serial terminal

### Step 3: Provision Device

1. Open **ESP RainMaker** app
2. Tap **"Add Device"**
3. **Scan QR code** from serial monitor
4. Follow app instructions:
   - Select your Wi-Fi network
   - Enter Wi-Fi password
   - Wait for connection (~30 seconds)

5. Device will appear in app as **"Environmental Logger"**

---

## ✅ Verification Checklist

### Hardware Verification

Connect to serial monitor and check logs:

```
☑ I (123) APP_DRIVER: === Initializing Hardware Drivers ===
☑ I (124) APP_DRIVER: I2C initialized successfully
☑ I (125) APP_DRIVER: GPIO initialized successfully
☑ I (126) APP_DRIVER: ADC initialized successfully
☑ I (130) DHT11: DHT11 initialized on GPIO4
☑ I (135) SSD1306: SSD1306 initialized successfully
☑ I (140) DISPLAY_TASK: Display task started
☑ I (145) SENSOR_TASK: Sensor task started
```

### Sensor Verification

Check that sensor readings appear:

```
☑ I (10000) SENSOR_TASK: DHT11: Temperature=25.5°C, Humidity=55.0%
☑ I (10001) SENSOR_TASK: Light Level: 2045/4095
☑ I (10002) SENSOR_TASK: AQI calculation: T=25.5, H=55.0, L=2045 → AQI=48
☑ I (10003) SENSOR_TASK: Sensor data sent to queue
```

### Cloud Verification

```
☑ I (15000) CLOUD_TASK: Received sensor data - T:25.5 H:55.0 AQI:48
☑ I (15001) CLOUD_TASK: Updated temperature: 25.5°C
☑ I (15002) CLOUD_TASK: Updated humidity: 55.0%
☑ I (15003) CLOUD_TASK: Updated AQI: 48
```

### Display Verification

- ✅ OLED shows "Env. Monitor" at top
- ✅ Temperature value displayed
- ✅ Humidity value displayed
- ✅ AQI value displayed
- ✅ Connection status shown ([W] or [C])

### Mobile App Verification

1. Open **ESP RainMaker** app
2. Find device **"Environmental Logger"**
3. Check 4 devices visible:
   - ✅ Temperature (with thresholds)
   - ✅ Humidity (with thresholds)
   - ✅ Air Quality (AQI + status)
   - ✅ Alert System (buzzer toggle)

---

## 🐛 Troubleshooting

### Issue: Build Fails with "RMAKER_PATH not found"

**Solution:**
```cmake
# Edit CMakeLists.txt, line 11:
set(RMAKER_PATH "C:/esp/esp-rainmaker")  # Use absolute path
```

### Issue: Flash Fails - "Port not found"

**Solution:**
```bash
# Windows: Check Device Manager → Ports (COM & LPT)
# Linux: Check available ports
ls /dev/ttyUSB*

# Install drivers (Windows):
# https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
```

### Issue: DHT11 Returns Invalid Data

**Solution:**
- Check wiring (especially 10kΩ pullup resistor)
- Verify GPIO4 connection
- Check serial logs for "Checksum error"
- Try another DHT11 sensor

### Issue: OLED Display Blank

**Solution:**
```bash
# Check I2C connection
# In serial monitor, look for:
I (xxx) SSD1306: SSD1306 initialized successfully

# If error, verify:
# - SDA → GPIO8
# - SCL → GPIO9
# - VCC → 3.3V (not 5V!)
# - GND → GND

# Test I2C address (should be 0x3C):
i2cdetect -y 0  # Linux
```

### Issue: Device Not Appearing in RainMaker App

**Solution:**
1. Erase flash completely:
   ```bash
   idf.py erase-flash
   idf.py flash
   ```

2. Check Wi-Fi credentials (2.4GHz only, not 5GHz)

3. Verify QR code scans correctly (try zooming in/out)

4. Check device logs for provisioning errors:
   ```
   I (xxx) PROV: Provisioning started
   ```

### Issue: "Partition table does not fit"

**Solution:**
```bash
# This project uses custom partitions.csv
# Verify file exists:
ls partitions.csv

# If missing, recreate it or use:
idf.py menuconfig
# → Partition Table → Custom partition table CSV
```

---

## 📊 Expected Serial Output (Sample)

```
I (300) APP_MAIN: === Smart Environmental Data Logger ===
I (301) APP_MAIN: Version: 1.0.0
I (350) APP_DRIVER: === Initializing Hardware Drivers ===
I (351) APP_DRIVER: I2C initialized successfully (SDA: GPIO8, SCL: GPIO9)
I (352) APP_DRIVER: GPIO initialized successfully
I (353) APP_DRIVER:   LED Green: GPIO2
I (354) APP_DRIVER:   LED Red:   GPIO3
I (355) APP_DRIVER:   Buzzer:    GPIO10
I (356) APP_DRIVER:   Button:    GPIO5
I (357) APP_DRIVER: ADC initialized successfully (GPIO0, Channel 0)
I (360) DHT11: DHT11 initialized on GPIO4
I (370) SSD1306: SSD1306 initialized successfully
I (380) DISPLAY_TASK: Display task started
I (390) SENSOR_TASK: Sensor task started
I (400) CLOUD_TASK: Cloud communication task started
I (410) ALERT_TASK: Alert monitoring task started
I (420) OTA_TASK: OTA monitoring task started
I (421) OTA_TASK: === Firmware Information ===
I (422) OTA_TASK: Project Name: smart_environmental_logger
I (423) OTA_TASK: Version: 1.0.0
I (424) OTA_TASK: Compile Date: Dec 16 2025
I (500) APP_MAIN: All tasks created successfully!
I (5000) wifi: wifi firmware version: xxx
I (5010) wifi: Starting WiFi provisioning...
I (10000) SENSOR_TASK: DHT11: Temperature=24.5°C, Humidity=58.0%
I (10001) SENSOR_TASK: Light Level: 2100/4095
I (10002) SENSOR_TASK: AQI: 52
I (10003) CLOUD_TASK: Received sensor data - T:24.5 H:58.0 AQI:52
I (10010) DISPLAY_TASK: Display updated
```

---

## 🎯 Next Steps After Successful Build

1. ✅ **Test Push Notifications:**
   - Heat sensor (blow hot air on DHT11)
   - Watch for alert in app

2. ✅ **Configure Voice Assistant:**
   - Follow Voice Assistant Setup Guide
   - Test "Hey Google" commands

3. ✅ **Test OTA Update:**
   - Change version in CMakeLists.txt
   - Build new firmware
   - Upload to RainMaker dashboard
   - Trigger OTA

4. ✅ **Check ESP Insights:**
   - Log in to insights dashboard
   - Verify metrics are being recorded

5. ✅ **Document Your Project:**
   - Take photos/videos
   - Create web-based report
   - Prepare for submission

---

## 📞 Getting Help

If you encounter issues:

1. **Check Logs:** Always read serial monitor output
2. **Verify Hardware:** Use multimeter to check connections
3. **ESP-IDF Docs:** https://docs.espressif.com/projects/esp-idf/
4. **RainMaker Docs:** https://rainmaker.espressif.com/docs/
5. **GitHub Issues:** Open issue in project repository

---

**Good luck with your build! 🚀**
