# Industrial Proactive Maintenance System – My Contribution

**Industrial Internship Project**  
**Organization**: Sanleva Prime Solutions  
**Duration**: June 2025 – July 2025  
**Team Size**: 5 members  
**Project Title**: Industry Proactive Maintenance System Using Sensor Data Monitoring and Cloud Forecasting

This repository showcases **my specific contributions** to the team project:  
- Power monitoring module (PZEM-004T)  
- Vibration monitoring module (MPU6050 → velocity mm/s RMS)  
- Central ESP-NOW + MQTT gateway (data aggregation & forwarding)

These components formed the electrical and mechanical health monitoring backbone, integrated with teammates' temperature (MAX6675) and RPM (A3144) modules, and visualized via Grafana + InfluxDB.

## Project Goal (Team Context)

Build a wireless IoT system for real-time and historical monitoring of industrial machine health parameters to enable **proactive/predictive maintenance** through early anomaly detection.

## Tech Stack (My Parts)

| Layer                | Technology / Library                  |
|----------------------|---------------------------------------|
| Microcontroller      | Glyph ESP32-C6                        |
| Vibration Sensor     | MPU6050_light                         |
| Power Meter          | PZEM004Tv30                           |
| Wireless Protocol    | ESP-NOW (peer-to-peer)                |
| Uplink               | Wi-Fi STA + PubSubClient (MQTT)       |
| Development          | Arduino framework                     |

## Hardware Used (My Modules)

- Glyph ESP32-C6 (sensor nodes)
- Glyph ESP32-C6 (gateway)
- MPU6050 (GY-521 module)
- PZEM-004T v3.0 (serial version)
- Stable 5 V supply per node

## Installation & Quick Start (My Modules)

1. Install **Arduino IDE** + ESP32 board support (v2.0.17 or newer)
2. Install required libraries:
   - MPU6050_light
   - PZEM004Tv30
   - PubSubClient
3. **Gateway**:
   - Upload `receiver.ino`
   - Open Serial Monitor and note the MAC address printed
4. **Sensor nodes**:
   - In `sender.ino`, update the `receiverAddress[]` array with the gateway's MAC address
   - Upload `sender.ino` to each sensor node
5. Power on the gateway → it connects to Wi-Fi and the MQTT broker
6. Power on sensor nodes → observe channel scanning and data transmission in Serial Monitor

## Results & Achievements

- Vibration classification correctly identified machine states (off / normal / abnormal) in lab and demo conditions
- Power readings (voltage, current, power, energy, frequency, PF) matched multimeter measurements within typical PZEM-004T tolerance
- Gateway maintained stable MQTT connection for >24 hours in testing
- System automatically recovered after intentional Wi-Fi channel changes on the gateway
- Enabled reliable team-wide data aggregation, feeding clean multi-sensor JSON to Grafana visualization

## Future Improvements (Team Scope)

- Add node ID / multi-sender support
- Implement TLS + authentication on MQTT
- OTA (Over-The-Air) firmware updates
- Deep sleep support for battery-powered nodes
- Cloud-based anomaly detection rules / ML models
- SMS / email alerts via Grafana notifications

## License

MIT License

## Acknowledgments

- Espressif – ESP-NOW implementation & ESP32/ESP32-C6 platform
- MPU6050_light library by rfetick
- PZEM004Tv30 library
- Teammates for temperature (MAX6675), RPM (A3144), and Grafana/InfluxDB pipeline development

---
**Author**: Moheshwar  
**Last updated**: January 2026  
**Internship Project**: Industry Proactive Maintenance System – Sanleva Prime Solutions