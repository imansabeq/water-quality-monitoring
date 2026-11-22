# Water Quality Monitoring (IoT): Arduino + ESP8266, simulated in Proteus and Cisco Packet Tracer

A university IoT project that monitors water quality (pH, turbidity, TDS, chlorine), decides whether the water is safe, and sends the readings toward a server. The hardware side is simulated in **Proteus** (Arduino UNO + sensors + ESP8266), and the network side (sensor node to gateway to IoT server) is simulated in **Cisco Packet Tracer (IoT mode)**.

![Proteus circuit](docs/circuit.png)
                                               ##Demo 



<img width="2389" height="1415" alt="demo" src="https://github.com/user-attachments/assets/de1263fb-0db6-49af-bb7b-fc04d6b4861b" />

## Why this project

Egypt's Vision 2030 puts weight on sustainable water management and smart infrastructure. Water quality is usually checked manually, at specific times and places. A continuous sensor node can report problems as they happen (a pH drop in a canal, a turbidity spike at a treatment plant intake) instead of weeks later.

## What is implemented (and what is not)

| Implemented | Not implemented (future work) |
|---|---|
| Proteus circuit: Arduino UNO, pH, turbidity, TDS, chlorine inputs, ESP8266 | Real hardware build and sensor calibration |
| Firmware: reads sensors, converts to units, checks safe limits, prints JSON | Actuators (relay, dosing pump, valve) |
| Data sent to the ESP8266 over SoftwareSerial | MQTT broker, database, dashboard (planned below) |
| Packet Tracer network: sensor node to Home Gateway to IoT Server | Modbus-RTU / 4-20 mA (discussed in the report only) |

**Important:** all readings in this repo come from simulation. The turbidity and TDS modules in Proteus do not output a signal, so they are driven by potentiometers (see [Challenges](#challenges-and-how-they-were-solved)).

## Architecture

```
 pH / turbidity / TDS / chlorine
              |  analog (A0-A3)
        Arduino UNO ---- USB Serial (9600) ----> JSON line per reading
              |
        SoftwareSerial (pins 10/11)
              |
          ESP8266 ---- Wi-Fi ----> Home Gateway ----> IoT Server   (Packet Tracer)
```

### Pin map

| Signal | Pin | Simulation input |
|---|---|---|
| pH | A0 | pH meter module (controlled by RV1) |
| Turbidity | A1 | Potentiometer (POT3) |
| TDS | A2 | Potentiometer (POT2) |
| Chlorine | A3 | Potentiometer (POT1) |
| ESP8266 RX / TX | 11 / 10 | SoftwareSerial |

## Firmware

File: [`firmware/water_monitor/water_monitor.ino`](firmware/water_monitor/water_monitor.ino)

- Averages 10 ADC readings per sensor to reduce noise.
- Converts voltage to units. **The conversion constants are tuned for the simulation** and live at the top of the file. A real build needs calibration (pH with buffer solutions, TDS with temperature compensation).
- The pH reading uses a moving average (50 samples at 200 ms, about 10 s), because the pH module's output swings in simulation.
- Safe limits: pH 6.5 to 8.5, chlorine 0.2 to 2.0 mg/L, turbidity up to 50 NTU, TDS up to 500 ppm. Every violated parameter is reported, not just the first.

Output on USB Serial, one JSON line every 2 seconds:

```json
{"ph":7.12,"chlorine":1.20,"turbidity":4.0,"tds":320,"safe":true,"reasons":[]}
{"ph":6.10,"chlorine":1.50,"turbidity":1.4,"tds":300,"safe":false,"reasons":["pH"]}
```

Line sent to the ESP8266 (same format as the original version):

```
PH=7.12&TURB=4&TDS=320&CHLOR=1.2
```

The first version of the sketch is kept in [`firmware/legacy/`](firmware/legacy/) for comparison.

## Packet Tracer (network side)

File: `packet-tracer/` (`.pkt`)

Packet Tracer's IoT library does not include pH, TDS or turbidity sensors, so generic sensors stand in for them. This simulation proves the **data path**: the MCU registers on the IoT Server over Wi-Fi through the Home Gateway. It is a separate simulation from the Proteus one; data does not flow between them.

Addressing used: MCU1 `192.168.25.100`, Server1 `192.168.25.102`, Home Gateway LAN `192.168.25.1`.

## How to run

**Proteus part**
1. Requires Proteus **8.17 or later** with the **AVR/microcontroller simulation licence**.
2. Open `proteus/iot_ass1_v2.pdsprj`.
3. Double-click **UNO1** and set *Program File* to `proteus/water_monitor.ino.hex`.
4. The pH meter module needs its own hex from the *pH Sensor Library for Proteus* (by The Engineering Projects). It is not included here; download it from the library's page and set it as the Program File of **PH2**.
5. Press Play. The Virtual Terminal (9600 baud) prints the JSON lines. Use the potentiometers to change turbidity, TDS and chlorine, and RV1 for pH.

**Rebuilding the firmware:** open `firmware/water_monitor/water_monitor.ino` in the Arduino IDE, select *Arduino Uno*, then *Sketch, Export Compiled Binary*, and use the `.hex` without `with_bootloader`.

![Safe reading](docs/terminal-safe.png)
![Unsafe reading](docs/terminal-unsafe.png)

## Challenges and how they were solved

| Problem | Solution |
|---|---|
| MCU1 could not reach Server1 in Packet Tracer | The Home Gateway itself was not set to `192.168.25.1`; fixed in its LAN settings |
| Raw ADC values (0 to 1023) are meaningless to a user | Convert to voltage, then to pH / ppm / NTU in `readSensors()` |
| pH, TDS and turbidity sensors exist in Proteus but not in Packet Tracer | Split the goals: Proteus for the hardware, Packet Tracer for the network, with generic stand-in sensors |
| No chlorine sensor in Proteus | Potentiometer on A3 simulates it |
| Project would not open (needs a newer Proteus) | Opened on Proteus 8.17+ |
| Compile error: `setup()` and `loop()` redefined | An old `.ino` was in the same sketch folder (Arduino merges all `.ino` files in a folder); moved it out |
| `Bad or missing Customer Key` for AVR2.DLL | The Proteus install had no AVR simulation licence; used a licensed one |
| Proteus could not find the hex files after the project was moved | Keep the hex files next to the project and re-select them in each part's *Program File* |
| Turbidity and TDS modules gave no signal (pins not modelled, constant readings) | Replaced their inputs with potentiometers on A1 and A2 |
| pH reading swung between about 6 and 8.6 with the control fixed | Added a 10 s moving average in the firmware |

## Future work

- MQTT pipeline with `docker-compose`: Mosquitto, InfluxDB, Grafana, plus a Python script that reads the Serial JSON and publishes it.
- GitHub Actions: compile the firmware with `arduino-cli` on every push.
- Actuators: relay-driven dosing pump or valve for chlorine and TDS correction.
- Real calibration on hardware.

## Repository structure

```
water-quality-monitoring/
├── README.md
├── docs/                 screenshots
├── firmware/
│   ├── water_monitor/    current firmware
│   └── legacy/           original sketch
├── proteus/              .pdsprj and compiled .hex
└── packet-tracer/        .pkt
```

## Credits

- pH meter model: *pH Sensor Library for Proteus*, The Engineering Projects.
- Course assignment: IoT, New Cairo Technological University.

**Author:** [Iman] · [https://www.linkedin.com/in/imansabeq/]
