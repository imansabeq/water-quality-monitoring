/*
  Water Quality Monitor (Arduino UNO + ESP8266)
  Sensors: pH, Turbidity, TDS, Chlorine (chlorine simulated with a potentiometer in Proteus)

  Outputs:
    - USB Serial (9600): one JSON line per reading, e.g.
        {"ph":7.12,"chlorine":1.20,"turbidity":4.0,"tds":320,"safe":true,"reasons":[]}
    - ESP8266 (SoftwareSerial): "PH=7.12&TURB=4&TDS=320&CHLOR=1.2" (same format as the original version)

  NOTE: the conversion constants below are tuned for the Proteus simulation.
  For real hardware, recalibrate each sensor (see README -> Calibration).
*/

#include <SoftwareSerial.h>

// ---------- Pins ----------
const uint8_t ESP_RX_PIN    = 11;
const uint8_t ESP_TX_PIN    = 10;
const uint8_t PH_PIN        = A0;
const uint8_t TURBIDITY_PIN = A1;
const uint8_t TDS_PIN       = A2;
const uint8_t CHLORINE_PIN  = A3;

SoftwareSerial espSerial(ESP_RX_PIN, ESP_TX_PIN);

// ---------- Settings ----------
#define HUMAN_READABLE 0                 // 1 = readable text on Serial, 0 = JSON only (for scripts)
const unsigned long SAMPLE_INTERVAL_MS = 2000;
const uint8_t ADC_SAMPLES = 10;          // average N readings to reduce noise

// pH smoothing: the pH module's output swings over time, so we keep a moving average
const unsigned long PH_SAMPLE_INTERVAL_MS = 200;   // how often a pH sample is taken
const uint8_t PH_AVG_SAMPLES = 50;                 // window = 50 x 200 ms = 10 s

// ---------- ADC ----------
const float ADC_REF_V = 5.0;
const float ADC_MAX   = 1023.0;

// ---------- Calibration (Proteus simulation values) ----------
const float PH_NEUTRAL_V   = 2.5;        // voltage at pH 7
const float PH_SLOPE       = -3.3;       // pH change per volt
const float CHLORINE_MAX   = 5.0;        // mg/L at full-scale (5V)
const float TDS_MAX_PPM    = 1000.0;     // ppm at full-scale (5V)

// Turbidity: voltage -> NTU, points ordered from high voltage (clear) to low voltage (dirty)
const uint8_t TURB_POINTS = 4;
const float TURB_V[TURB_POINTS]   = {4.1, 3.4, 2.8, 2.5};
const float TURB_NTU[TURB_POINTS] = {0.0, 10.0, 100.0, 400.0};
const float TURB_MAX_NTU = 500.0;        // below the last point

// ---------- Safe limits ----------
const float PH_MIN = 6.5,       PH_MAX = 8.5;
const float CHLORINE_MIN = 0.2, CHLORINE_MAX_SAFE = 2.0;
const float TURBIDITY_MAX = 50.0;
const float TDS_MAX_SAFE = 500.0;

// Bit flags for unsafe reasons
const uint8_t BAD_PH = 1, BAD_CHLORINE = 2, BAD_TURBIDITY = 4, BAD_TDS = 8;

float phValue = 0.0;
float chlorineValue = 0.0;
float turbidityValue = 0.0;
float tdsValue = 0.0;
unsigned long lastSample = 0;

float phBuffer[PH_AVG_SAMPLES];
uint8_t phCount = 0;                     // samples stored so far
uint8_t phIndex = 0;                     // next slot to overwrite
unsigned long lastPhSample = 0;

void setup() {
  Serial.begin(9600);
  espSerial.begin(9600);
#if HUMAN_READABLE
  Serial.println(F("Water Monitor - ESP8266 Mode"));
  Serial.println(F("System Ready."));
#endif
  delay(2000);
}

void loop() {
  samplePh();                            // keeps running between reports
  if (millis() - lastSample < SAMPLE_INTERVAL_MS) return;
  lastSample = millis();

  readSensors();
  uint8_t flags = checkWater();
  printSerial(flags);
  sendDataToESP();
}

// ---------- Reading ----------
float readVoltage(uint8_t pin) {
  long sum = 0;
  for (uint8_t i = 0; i < ADC_SAMPLES; i++) {
    sum += analogRead(pin);
    delay(2);
  }
  return (sum / (float)ADC_SAMPLES) * (ADC_REF_V / ADC_MAX);
}

// Linear interpolation through the turbidity calibration points
float turbidityFromVoltage(float v) {
  if (v >= TURB_V[0]) return TURB_NTU[0];
  for (uint8_t i = 0; i < TURB_POINTS - 1; i++) {
    if (v >= TURB_V[i + 1]) {
      float t = (v - TURB_V[i + 1]) / (TURB_V[i] - TURB_V[i + 1]);
      return TURB_NTU[i + 1] + t * (TURB_NTU[i] - TURB_NTU[i + 1]);
    }
  }
  return TURB_MAX_NTU;
}

// Take a pH voltage sample every PH_SAMPLE_INTERVAL_MS into a ring buffer
void samplePh() {
  if (millis() - lastPhSample < PH_SAMPLE_INTERVAL_MS) return;
  lastPhSample = millis();
  phBuffer[phIndex] = readVoltage(PH_PIN);
  phIndex = (phIndex + 1) % PH_AVG_SAMPLES;
  if (phCount < PH_AVG_SAMPLES) phCount++;
}

float averagePhVoltage() {
  if (phCount == 0) return readVoltage(PH_PIN);   // no samples yet
  float sum = 0;
  for (uint8_t i = 0; i < phCount; i++) sum += phBuffer[i];
  return sum / phCount;
}

void readSensors() {
  float phV = averagePhVoltage();
  phValue = 7.0 + (phV - PH_NEUTRAL_V) * PH_SLOPE;

  float clV = readVoltage(CHLORINE_PIN);
  chlorineValue = (clV / ADC_REF_V) * CHLORINE_MAX;

  turbidityValue = turbidityFromVoltage(readVoltage(TURBIDITY_PIN));

  float tdsV = readVoltage(TDS_PIN);
  tdsValue = (tdsV / ADC_REF_V) * TDS_MAX_PPM;
}

// ---------- Analysis ----------
uint8_t checkWater() {
  uint8_t flags = 0;
  if (phValue < PH_MIN || phValue > PH_MAX) flags |= BAD_PH;
  if (chlorineValue < CHLORINE_MIN || chlorineValue > CHLORINE_MAX_SAFE) flags |= BAD_CHLORINE;
  if (turbidityValue > TURBIDITY_MAX) flags |= BAD_TURBIDITY;
  if (tdsValue > TDS_MAX_SAFE) flags |= BAD_TDS;
  return flags;
}

// ---------- Output ----------
void printReasons(uint8_t flags) {
  bool first = true;
  if (flags & BAD_PH)        { Serial.print(F("\"pH\""));        first = false; }
  if (flags & BAD_CHLORINE)  { if (!first) Serial.print(','); Serial.print(F("\"Chlorine\""));  first = false; }
  if (flags & BAD_TURBIDITY) { if (!first) Serial.print(','); Serial.print(F("\"Turbidity\"")); first = false; }
  if (flags & BAD_TDS)       { if (!first) Serial.print(','); Serial.print(F("\"TDS\"")); }
}

void printSerial(uint8_t flags) {
#if HUMAN_READABLE
  Serial.println();
  Serial.print(F("PH: "));        Serial.println(phValue);
  Serial.print(F("Chlorine: "));  Serial.println(chlorineValue);
  Serial.print(F("Turbidity: ")); Serial.println(turbidityValue);
  Serial.print(F("TDS: "));       Serial.println(tdsValue);
  Serial.print(F("Safe Water: ")); Serial.println(flags == 0 ? F("SAFE") : F("UNSAFE"));
  Serial.println(F("----------"));
#else
  Serial.print(F("{\"ph\":"));        Serial.print(phValue, 2);
  Serial.print(F(",\"chlorine\":"));  Serial.print(chlorineValue, 2);
  Serial.print(F(",\"turbidity\":")); Serial.print(turbidityValue, 1);
  Serial.print(F(",\"tds\":"));       Serial.print(tdsValue, 0);
  Serial.print(F(",\"safe\":"));      Serial.print(flags == 0 ? F("true") : F("false"));
  Serial.print(F(",\"reasons\":["));  printReasons(flags);
  Serial.println(F("]}"));
#endif
}

void sendDataToESP() {
  espSerial.print(F("PH="));      espSerial.print(phValue, 2);
  espSerial.print(F("&TURB="));   espSerial.print(turbidityValue, 0);
  espSerial.print(F("&TDS="));    espSerial.print(tdsValue, 0);
  espSerial.print(F("&CHLOR="));  espSerial.println(chlorineValue, 1);
}
