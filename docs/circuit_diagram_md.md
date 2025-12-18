# Circuit Diagram - Seeed Studio XIAO ESP32-C3

## Pinout Reference

### Seeed Studio XIAO ESP32-C3 Pinout

```
           USB-C
            ___
           |   |
           |___|
    ___________________
   |                   |
   | D0  (GPIO0)  5V   |  ← Power out
   | D1  (GPIO1)  GND  |  ← Ground
   | D2  (GPIO2)  3V3  |  ← 3.3V out
   | D3  (GPIO3)  D10  |  (GPIO10)
   | D4  (GPIO4)  D9   |  (GPIO9) SCL
   | D5  (GPIO5)  D8   |  (GPIO8) SDA
   | D6  (GPIO6)  D7   |  (GPIO7)
   |___________________|
         XIAO-C3
```

## Complete Wiring Diagram

### Power Distribution
```
XIAO 3.3V → Power Rail (+)
XIAO GND  → Ground Rail (-)
```

### DHT11 Temperature & Humidity Sensor
```
DHT11 Pin 1 (VCC)  → 3.3V
DHT11 Pin 2 (DATA) → GPIO4 (D4)
DHT11 Pin 2 (DATA) → 10kΩ Resistor → 3.3V (pullup)
DHT11 Pin 4 (GND)  → GND
```

### LDR (Light Sensor) - Voltage Divider
```
3.3V → LDR → GPIO0 (D0/A0) → 10kΩ → GND
                |
            (ADC Input)
```

### OLED Display (SSD1306 - 128x64 I2C)
```
OLED VCC → 3.3V (NOT 5V!)
OLED GND → GND
OLED SDA → GPIO8 (D8)
OLED SCL → GPIO9 (D9)
```

### Green LED (Status Indicator)
```
GPIO2 (D2) → 220Ω → LED+ → LED- → GND
```

### Red LED (Alert Indicator)
```
GPIO3 (D3) → 220Ω → LED+ → LED- → GND
```

### Active Buzzer (Audio Alert)
```
GPIO10 (D10) → Buzzer+ → Buzzer- → GND
```

### Push Button (Manual Control)
```
3.3V → 10kΩ → GPIO5 (D5) → Button → GND
               |
          (Pulled High)
```

## Breadboard Layout (Text Diagram)

```
                        3.3V Rail (+)
    ═══════════════════════════════════════════════
    
    DHT11               OLED            LEDs
    ┌───┐              ┌────┐          🔴 Red
    │ 1 │→ 3.3V        │VCC │→ 3.3V    ↑
    │ 2 │→ GPIO4       │GND │→ GND     220Ω
    │ 3 │  NC          │SCL │→ GPIO9   ↓
    │ 4 │→ GND         │SDA │→ GPIO8   GPIO3
    └───┘              └────┘
      ↑                               🟢 Green
      │                               ↑
     10kΩ                            220Ω
      │                               ↓
    3.3V                             GPIO2

    LDR Circuit         Button         Buzzer
    ┌───┐              ┌────┐         ┌───┐
    3.3V→│LDR│→GPIO0   │BTN │         │🔊 │
         └───┘    ↓    └────┘         └───┘
                 10kΩ    ↓             ↑
                  ↓     GND          GPIO10
                 GND                   ↓
                                      GND
    
    ═══════════════════════════════════════════════
                        GND Rail (-)
```

## Component Specifications

| Component | Rating | Notes |
|-----------|--------|-------|
| DHT11 | 3-5.5V | Use 3.3V for XIAO |
| OLED SSD1306 | 3.3V | **NOT 5V!** Will damage at 5V |
| LDR | Any | Common photoresistor |
| LEDs | 2-3V, 20mA | Use 220Ω current limiting |
| Buzzer | 3-5V | Active buzzer (has oscillator) |
| Button | Any | Momentary push button |
| Resistors | 10kΩ (3×) | Pullups and dividers |
| Resistors | 220Ω (2×) | LED current limiting |

## Critical Notes ⚠️

1. **OLED Voltage:** Use 3.3V ONLY! 5V will destroy the display
2. **DHT11 Pullup:** 10kΩ resistor between DATA and 3.3V is mandatory
3. **GPIO Limits:** XIAO ESP32-C3 GPIOs are 3.3V tolerant only
4. **Current Limits:** Total GPIO current should not exceed 40mA per pin
5. **USB Power:** XIAO can be powered via USB-C (no external supply needed)

## Testing Individual Components

### Test 1: OLED Display
```c
// Flash basic test
ssd1306_draw_string(0, 0, "Hello", 16, 1);
ssd1306_refresh_gram();
```

### Test 2: DHT11 Sensor
```c
float temp, hum;
if (dht11_read(&temp, &hum) == ESP_OK) {
    printf("T: %.1f, H: %.1f\n", temp, hum);
}
```

### Test 3: LDR Sensor
```c
int adc_value = adc1_get_raw(ADC1_CHANNEL_0);
printf("Light: %d/4095\n", adc_value);
```

### Test 4: LEDs
```c
gpio_set_level(GPIO_NUM_2, 1); // Green ON
gpio_set_level(GPIO_NUM_3, 1); // Red ON
```

## Troubleshooting

| Issue | Cause | Solution |
|-------|-------|----------|
| OLED blank | Wrong voltage or wiring | Check 3.3V, verify I2C address 0x3C |
| DHT11 timeout | Missing pullup | Add 10kΩ resistor DATA→3.3V |
| LDR always 4095 | Open circuit | Check LDR and 10kΩ connections |
| LEDs dim | Low current | Reduce resistor to 150Ω |
| Buzzer quiet | Passive type used | Use **active** buzzer with oscillator |

## Power Budget

| Component | Current | Notes |
|-----------|---------|-------|
| XIAO ESP32-C3 | ~80mA | Wi-Fi active |
| OLED Display | ~20mA | Full brightness |
| DHT11 | ~2.5mA | During read |
| LEDs (both) | ~40mA | 20mA each |
| Buzzer | ~30mA | When active |
| **TOTAL** | **~172mA** | Well within USB 500mA limit |

USB 2.0 provides 500mA, so this circuit is safe to power via USB-C.

---

**Save this diagram and refer to it during assembly!**
