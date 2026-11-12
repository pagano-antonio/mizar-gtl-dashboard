# 🚐 MIZAR GTL – Camper Control Center

A touchscreen-based monitoring and control dashboard for a camper, powered by an **ESP32-S3** and the **ELECROW CrowPanel 7-inch display**.

MIZAR GTL aims to provide a modern digital control panel for monitoring the camper's water tanks, batteries, and electrical appliances through an intuitive graphical interface.

The project is developed using Arduino IDE, ESP32 and LVGL.

**GitHub Repository:**  
https://github.com/pagano-antonio/mizar-gtl-dashboard

## 📸 Dashboard

The dashboard is designed for an 800×480 touchscreen display and is organized into four main areas.

### 1. Header and system information

- Camper logo and project name.
- Wi-Fi connection status.
- Current time and date.
- Settings button.

### 2. Water tank monitoring

Monitoring of three independent tanks:

| Tank | Initial simulated level |
|---|---:|
| Fresh Water Tank 1 | 75% |
| Fresh Water Tank 2 | 60% |
| Grey Water Tank | 35% |

Each tank includes:
- Graphical water-level indicator.
- Numeric percentage.
- Horizontal progress bar.
- Independent identification.

Water levels are currently simulated and will be connected to real sensors in a future development phase.

### 3. Battery monitoring

Independent monitoring of two camper batteries:

| Battery | Initial simulated voltage |
|---|---:|
| Leisure Battery | 12.8 V |
| Starter Battery | 12.6 V |

Each battery includes:
- Voltage display.
- Dedicated graphical icon.
- Multi-segment charge indicator.
- Independent identification.

Battery readings are currently simulated.

### 4. Appliance control

Three touchscreen controls are available:

| Appliance | Initial state |
|---|---|
| Interior Lights | ON |
| Water Pump | OFF |
| Exterior Light | ON |

The switches allow users to change each appliance's state through the touchscreen.

**Note:** Switches currently provide graphical interaction only. They do not control physical relays or the camper's electrical circuits.

---

## 🧰 Hardware

| Component | Description |
|---|---|
| Microcontroller | ESP32-S3 |
| Display | ELECROW CrowPanel 7" |
| Resolution | 800 × 480 pixels |
| Touchscreen | Capacitive |
| Connectivity | 2.4 GHz Wi-Fi |
| Development power supply | USB-C 5V |

## 💻 Software and Technologies

- Arduino IDE 2.x
- ESP32 Arduino Core
- LVGL 9.1.0
- LovyanGFX (ELECROW-provided modified version)
- C/C++

## 🏗️ Project Architecture

The software is organized to separate hardware initialization from graphical interface management.

Main components include:

- **Display Driver:** RGB display initialization and management.
- **Touch Driver:** touchscreen input handling.
- **LVGL:** graphical interface rendering.
- **Dashboard UI:** monitoring cards and appliance controls.
- **Data Model:** simulated values for tanks, batteries and appliances.

## 🚀 Installation and Setup

### Requirements

- macOS, Windows or Linux.
- Arduino IDE 2.x.
- ESP32 Arduino Core.
- LVGL and the ELECROW-compatible LovyanGFX library.

### Installation Steps

1. Clone the repository:

   ```bash
   git clone https://github.com/pagano-antonio/mizar-gtl-dashboard.git
   ```

2. Open the project in Arduino IDE.

3. Make sure the required libraries and the ELECROW-compatible LovyanGFX version are installed.

4. Select the board:

   `ESP32S3 Dev Module`

5. Configure the board settings according to the project:

   | Setting | Value |
   |---|---|
   | CPU Frequency | 240 MHz |
   | Flash Size | 4 MB |
   | PSRAM | OPI PSRAM |
   | Partition Scheme | Huge APP |

6. Connect the CrowPanel using USB-C.

7. Select the correct serial port.

8. Compile and upload the firmware.

**Important:** This project uses ELECROW-specific RGB framebuffer functionality. The standard LovyanGFX library may not provide the required methods. Use the compatible ELECROW version supplied with the original display examples.

## 🔧 Development Status

| Feature | Status |
|---|---|
| Display initialization | Completed |
| Touchscreen support | Completed |
| Graphical dashboard | Completed |
| Fresh Water Tank 1 monitoring | Simulated |
| Fresh Water Tank 2 monitoring | Simulated |
| Grey Water Tank monitoring | Simulated |
| Leisure Battery monitoring | Simulated |
| Starter Battery monitoring | Simulated |
| Interior Lights switch | Simulated |
| Water Pump switch | Simulated |
| Exterior Light switch | Simulated |
| Wi-Fi connectivity | To be completed |
| Real sensor integration | Planned |
| Physical relay control | Planned |

## 🗺️ Future Development

Planned improvements include:

- Integration of real water-level sensors.
- Leisure battery voltage monitoring.
- Starter battery voltage monitoring.
- Integration of suitable relay modules.
- Physical control of lights and water pump.
- Wi-Fi connectivity.
- Remote dashboard access from smartphones and tablets.
- Configurable camper settings.
- Alerts and notifications.

## ⚠️ Safety

This project is currently a prototype.

Before connecting the system to the camper's electrical installation, appropriate electrical protections must be implemented, including:

- A suitable DC-DC converter for powering the display and ESP32.
- Proper fuses and circuit protection.
- Relay modules rated for the actual electrical loads.
- Appropriate separation and protection of 12V circuits.
- Electrical compatibility checks for all sensors.

**Never connect camper 12V circuits directly to ESP32 GPIO pins.**

The original camper control panel should remain operational independently of this system.

## 📄 License

License to be defined.

---

**MIZAR GTL – Camper Control Center**

A personal project bringing digital monitoring and smart control to the camper experience.
