# 🌱 AgriAid

### Autonomous Smart Irrigation & Plant Monitoring System

AgriAid is an ESP32-S3-based smart agriculture system designed to monitor plant and environmental conditions, automate irrigation, collect agricultural data, and provide a foundation for TinyML-based decision making.

The system combines embedded sensing, automated irrigation control, IoT monitoring, local data logging, and TinyML on an ESP32-S3.

---

## 🚀 Project Overview

AgriAid is being developed as an autonomous irrigation and plant monitoring prototype for agricultural applications.

The system continuously monitors parameters such as:

- 🌱 Soil moisture
- 🌡️ Temperature
- 💧 Relative humidity
- 💦 Water flow
- ☀️ Day/night conditions

Based on these inputs, the controller can operate an irrigation pump while applying safety conditions such as maximum pump runtime, cooldown periods, and night-time irrigation restrictions.

The project also provides a data collection pipeline for developing and evaluating TinyML models for agricultural decision-making.

---

## 🧠 Technology Stack

### Hardware

- ESP32-S3-N16R8
- Capacitive soil moisture sensor
- DHT22 temperature & humidity sensor
- YF-S201 water flow sensor
- LDR light sensor
- 5V relay module
- DC submersible water pump
- PCF8575 I/O expander
- 2× 18650 battery system
- USB-C PD power system
- DC-DC buck converter

### Firmware

- ESP-IDF
- C / C++
- FreeRTOS
- ESP32-S3 PSRAM
- TensorFlow Lite Micro
- ESP-NN
- ESP-IDF LED Strip driver
- LittleFS
- Embedded HTTP server

---

## 🏗️ System Architecture

```text
                 ┌──────────────────────┐
                 │      ESP32-S3        │
                 │      N16R8           │
                 └──────────┬───────────┘
                            │
          ┌─────────────────┼─────────────────┐
          │                 │                 │
          ▼                 ▼                 ▼
     Environmental      Irrigation         TinyML
       Sensors           Control          Inference
          │                 │                 │
          │                 ▼                 │
          │              Relay ──► Pump       │
          │                                   │
          └──────────────┬────────────────────┘
                         │
                         ▼
                  Data Logging
                         │
                         ▼
                     LittleFS
                         │
                         ▼
                  CSV Sensor Data

                         │
                         ▼
                  Web Dashboard
