# 💧 Water Quality Monitoring System (IoT)

**Arduino UNO + ESP8266 · Simulated in Proteus & Cisco Packet Tracer**

![Arduino](https://img.shields.io/badge/Hardware-Arduino%20UNO%20%2B%20ESP8266-00979D?style=flat&logo=arduino&logoColor=white)
![Proteus](https://img.shields.io/badge/Simulation-Proteus%208.17%2B-00599C?style=flat)
![Packet Tracer](https://img.shields.io/badge/Network-Cisco%20Packet%20Tracer-1BA0D7?style=flat&logo=cisco&logoColor=white)
![Status](https://img.shields.io/badge/Project-University%20IoT%20Assignment-success?style=flat)

A university IoT project that monitors water quality parameters (**pH, turbidity, TDS, and chlorine**), evaluates whether the water is safe against standard thresholds, and transmits readings toward an IoT server. The hardware layer is simulated in **Proteus**, while the network path (sensor node → gateway → IoT server) is simulated in **Cisco Packet Tracer (IoT mode)**.

---

## 📌 Table of Contents
- [Why This Project](#-why-this-project)
- [Demo & Circuit](#-demo--circuit)
- [Project Scope](#-project-scope)
- [Architecture & Pin Map](#-architecture--pin-map)
- [Firmware & Data Format](#-firmware--data-format)
- [Network Simulation (Packet Tracer)](#-network-simulation-packet-tracer)
- [How to Run](#-how-to-run)
- [Challenges & Solutions](#-challenges--solutions)
- [Future Work](#-future-work)
- [Repository Structure](#-repository-structure)
- [Credits & Author](#-credits--author)

---

## 🌍 Why This Project

Egypt's **Vision 2030** places strong emphasis on sustainable water management and smart infrastructure. Traditionally, water quality is tested manually at fixed intervals and locations. A continuous IoT monitoring node can report anomalies the moment they happen—such as a sudden pH drop in a canal or a turbidity spike at a treatment plant intake—rather than discovering the issue weeks later.

---

## 🎥 Demo & Circuit

### Demo Preview
<img width="2389" height="1415" alt="Water Quality Monitoring Demo" src="https://github.com/user-attachments/assets/de1263fb-0db6-49af-bb7b-fc04d6b4861b" />

### Proteus Circuit Schematic
![Proteus circuit](docs/circuit.png)

---

## 📋 Project Scope

| Implemented | Not Implemented (Future Work) |
|---|---|
| Proteus circuit: Arduino UNO, pH, turbidity, TDS, chlorine inputs, ESP8266 | Physical hardware build and wet-lab sensor calibration |
| Firmware: sensor sampling, unit conversion, threshold checks, JSON serial output | Actuators (relays, dosing pumps, solenoid valves) |
| Data transmission from Arduino to ESP8266 over `SoftwareSerial` | MQTT broker, time-series database, and live dashboard |
| Packet Tracer network: MCU node → Home Gateway → IoT Server | Industrial protocols such as Modbus-RTU / 4–20 mA (covered in report only) |

> [!IMPORTANT]
> **Simulation Notice:** All readings in this repository come from simulation. Because the Proteus turbidity and TDS modules do not output active analog signals in simulation, potentiometers are used to drive their inputs (see [Challenges & Solutions](#-challenges--solutions)).

---

## 🏗️ Architecture & Pin Map

```text
  pH / Turbidity / TDS / Chlorine
               │  Analog (A0–A3)
               ▼
          Arduino UNO ──── USB Serial (9600 baud) ────▶ JSON line per reading
               │
         SoftwareSerial (Pins 10 / 11)
               │
               ▼
            ESP8266 ──── Wi-Fi ────▶ Home Gateway ────▶ IoT Server (Packet Tracer)
```

### Pin Mapping

| Signal | Arduino Pin | Simulation Input |
|---|---|---|
| **pH** | `A0` | pH meter module (controlled by `RV1`) |
| **Turbidity** | `A1` | Potentiometer (`POT3`) |
| **TDS** | `A2` | Potentiometer (`POT2`) |
| **Chlorine** | `A3` | Potentiometer (`POT1`) |
| **ESP8266 TX / RX** | `10` / `11` | `SoftwareSerial` (Arduino RX = 10, TX = 11) |

---

## 💻 Firmware & Data Format

**Sketch:** [`firmware/water_monitor/water_monitor.ino`](firmware/water_monitor/water_monitor.ino)  
*(The initial version of the sketch is archived in [`firmware/legacy/`](firmware/legacy/) for comparison.)*

### Key Firmware Features
- **Noise Reduction:** Averages 10 ADC readings per sensor channel.
- **pH Stabilization:** Applies a 50-sample moving average (sampled every `200 ms`, ~10 seconds window) to smooth out fluctuations from the simulated pH module.
- **Unit Conversion:** Converts raw ADC values (`0–1023`) to voltage and physical units. Conversion constants are defined at the top of the sketch and tuned for simulation *(a real-world build requires pH buffer calibration and TDS temperature compensation)*.
- **Full Violation Reporting:** Checks all parameters against safe thresholds and lists **every** violated parameter in the output, not just the first one.

### Safe Water Thresholds

| Parameter | Safe Range | Unit |
|---|---|---|
| **pH** | `6.5` – `8.5` | pH |
| **Chlorine** | `0.2` – `2.0` | mg/L |
| **Turbidity** | `≤ 50` | NTU |
| **TDS** | `≤ 500` | ppm |

### Sample Outputs

**1. USB Serial Output** (one JSON line every 2 seconds):
```json
{"ph":7.12,"chlorine":1.20,"turbidity":4.0,"tds":320,"safe":true,"reasons":[]}
{"ph":6.10,"chlorine":1.50,"turbidity":1.4,"tds":300,"safe":false,"reasons":["pH"]}
```

**2. ESP8266 `SoftwareSerial` Payload** (preserving the legacy format):
```text
PH=7.12&TURB=4&TDS=320&CHLOR=1.2
```

---

## 🌐 Network Simulation (Packet Tracer)

**File:** `packet-tracer/` (`.pkt`)

Cisco Packet Tracer's IoT library does not include dedicated pH, TDS, or turbidity sensors, so generic IoT sensors are used as stand-ins. This simulation validates the **end-to-end network path**, showing the MCU registering on the IoT Server over Wi-Fi through the Home Gateway.

> [!NOTE]
> The Proteus and Packet Tracer simulations run independently; live data does not bridge between the two tools.

### IP Addressing Scheme

| Device | IP Address | Role |
|---|---|---|
| **Home Gateway (LAN)** | `192.168.25.1` | Local Wi-Fi router / default gateway |
| **MCU1** | `192.168.25.100` | IoT sensor node |
| **Server1** | `192.168.25.102` | IoT Registration Server |

---

## 🚀 How to Run

### Running the Proteus Simulation
1. Ensure **Proteus 8.17 or later** is installed with an active **AVR / Microcontroller simulation license**.
2. Open `proteus/iot_ass1_v2.pdsprj`.
3. Double-click **`UNO1`** and set *Program File* to `proteus/water_monitor.ino.hex`.
4. Download the hex file from the *pH Sensor Library for Proteus* (by *The Engineering Projects*)—not included in this repo—and assign it as the *Program File* for **`PH2`**.
5. Press **Play**. The Virtual Terminal (`9600 baud`) will print JSON readings every 2 seconds. Adjust `POT1`–`POT3` for chlorine, TDS, and turbidity, and `RV1` for pH.

### Rebuilding the Firmware (Optional)
1. Open `firmware/water_monitor/water_monitor.ino` in the **Arduino IDE**.
2. Select **Board: Arduino Uno**.
3. Go to **Sketch → Export Compiled Binary**.
4. Use the generated `.hex` file **without** `with_bootloader` in Proteus.

### Terminal Output Examples

| Safe Reading | Unsafe Reading |
| :---: | :---: |
| ![Safe reading](docs/terminal-safe.png) | ![Unsafe reading](docs/terminal-unsafe.png) |

---

## 🛠️ Challenges & Solutions

| Challenge | Solution |
|---|---|
| `MCU1` could not reach `Server1` in Packet Tracer | Configured the Home Gateway LAN IP to `192.168.25.1` to match the subnet |
| Raw ADC values (`0–1023`) are not human-readable | Converted ADC counts to voltage, then to pH / ppm / NTU / mg/L inside `readSensors()` |
| pH, TDS, and turbidity sensors exist in Proteus but not in Packet Tracer | Separated scope: Proteus models the hardware/sensing; Packet Tracer models the network using generic sensors |
| No chlorine sensor model available in Proteus | Simulated the chlorine sensor output using a potentiometer on `A3` |
| Project failed to open on older Proteus builds | Upgraded and opened on **Proteus 8.17+** |
| Compiler error: `setup()` and `loop()` redefined | Removed a duplicate `.ino` file from the same sketch folder (Arduino IDE merges all `.ino` files in one directory) |
| `Bad or missing Customer Key` error for `AVR2.DLL` | Switched to a Proteus installation with a valid AVR simulation license |
| Proteus lost `.hex` file paths after moving the project folder | Kept `.hex` files alongside the `.pdsprj` file and re-linked each component's *Program File* |
| Turbidity and TDS modules produced static/no output in Proteus | Replaced their unmodeled output pins with potentiometers on `A1` and `A2` |
| pH readings oscillated between `~6.0` and `~8.6` at a fixed input | Implemented a 10-second moving average (50 samples) in firmware |

---

## 🔮 Future Work

- **Telemetry Pipeline (`docker-compose`):** Mosquitto (MQTT broker) + InfluxDB + Grafana, fed by a Python script reading the Serial JSON stream.
- **CI/CD:** GitHub Actions workflow using `arduino-cli` to automatically compile and verify the firmware on every push.
- **Automated Control:** Relay-driven dosing pumps and valves for automatic chlorine and TDS correction.
- **Hardware Prototyping:** Building and calibrating the physical circuit with real water samples.

---

## 📁 Repository Structure

```text
water-quality-monitoring/
├── README.md
├── docs/                 # Circuit schematic and terminal screenshots
├── firmware/
│   ├── water_monitor/    # Current Arduino firmware (.ino)
│   └── legacy/           # Original sketch for comparison
├── proteus/              # Proteus schematic (.pdsprj) and compiled .hex
└── packet-tracer/        # Cisco Packet Tracer simulation (.pkt)
```

---

## 🙌 Credits & Author

- **pH Meter Simulation Model:** *pH Sensor Library for Proteus* by *The Engineering Projects*.
- **Academic Context:** IoT Course Assignment — **New Cairo Technological University**.

**Author:** Iman · [LinkedIn](https://www.linkedin.com/in/imansabeq/)
