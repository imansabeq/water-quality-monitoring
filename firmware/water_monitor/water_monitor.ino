#include <SoftwareSerial.h> 
#include <math.h> 

// 0 = readable text on the Serial Monitor / Virtual Terminal (default)
// 1 = one JSON line per reading (for scripts, e.g. the MQTT pipeline)
#define JSON_OUTPUT 1

const int ESP_RX_PIN = 11; 
const int ESP_TX_PIN = 10; 
SoftwareSerial espSerial(ESP_RX_PIN, ESP_TX_PIN); 

const int PH_PIN = A0;
const int CHLORINE_PIN = A3; 
const int TURBIDITY_PIN = A1;
const int TDS_PIN = A2;

float phValue = 0.0;
float chlorineValue = 0.0;
float turbidityValue = 0.0;
float tdsValue = 0.0;

// How often a full set of readings is printed / sent to the ESP (in milliseconds)
const unsigned long REPORT_INTERVAL_MS = 2000;
unsigned long lastReport = 0;

// pH smoothing (moving average): take a pH sample every 200 ms and average the last 50 (about 10 s)
const unsigned long PH_SAMPLE_INTERVAL_MS = 200;
const int PH_AVG_SAMPLES = 50;
float phBuffer[PH_AVG_SAMPLES];
int phCount = 0;
int phIndex = 0;
unsigned long lastPhSample = 0;

void setup() {
  Serial.begin(9600);
  
  espSerial.begin(9600); 
  
  Serial.println("Water Monitor - ESP8266 Mode");
  Serial.println("Initializing...");
  delay(2000);
  Serial.println("System Ready.");
}

void loop() {
  samplePh();                                   // keeps the pH average updating between reports

  if (millis() - lastReport < REPORT_INTERVAL_MS) return;
  lastReport = millis();

  readSensors();

  analyzeAndDebug();

  sendDataToESP();
}

// Take a pH voltage sample every PH_SAMPLE_INTERVAL_MS into a ring buffer
void samplePh() {
  if (millis() - lastPhSample < PH_SAMPLE_INTERVAL_MS) return;
  lastPhSample = millis();

  phBuffer[phIndex] = analogRead(PH_PIN) * (5.0 / 1023.0);
  phIndex = (phIndex + 1) % PH_AVG_SAMPLES;
  if (phCount < PH_AVG_SAMPLES) phCount++;
}

float averagePhVoltage() {
  if (phCount == 0) return analogRead(PH_PIN) * (5.0 / 1023.0);   // no samples yet
  float sum = 0.0;
  for (int i = 0; i < phCount; i++) sum += phBuffer[i];
  return sum / phCount;
}

void readSensors() {
  float phVoltage = averagePhVoltage();
  phValue = 7.0 + ((2.5 - phVoltage) * 3.3); 
  
  int chlorineRaw = analogRead(CHLORINE_PIN);
  chlorineValue = map(chlorineRaw, 0, 1023, 0, 500) / 100.0;

  int turbidityRaw = analogRead(TURBIDITY_PIN);
  float turbidityVoltage = turbidityRaw * (5.0 / 1023.0);
  
  if (turbidityVoltage >= 4.1) {
    turbidityValue = 0.0;
  } else if (turbidityVoltage < 4.1 && turbidityVoltage >= 3.4) {
    turbidityValue = map((long)(turbidityVoltage * 100), 340, 410, 10, 0); 
  } else if (turbidityVoltage < 3.4 && turbidityVoltage >= 2.8) {
    turbidityValue = map((long)(turbidityVoltage * 100), 280, 340, 100, 10);
  } else if (turbidityVoltage < 2.8 && turbidityVoltage >= 2.5) {
    turbidityValue = map((long)(turbidityVoltage * 100), 250, 280, 400, 100);
  } else {
    turbidityValue = 500.0; 
  }
  if (turbidityValue < 0) turbidityValue = 0;

  int tdsRaw = analogRead(TDS_PIN);
  float tdsVoltage = tdsRaw * (5.0 / 1023.0);
  tdsValue = (tdsVoltage / 5.0) * 1000.0; 
}

void addReason(String &reasons, const char* name) {
  if (reasons.length() > 0) reasons += ", ";
  reasons += name;
}

void analyzeAndDebug() {
  bool isWaterSafe = true;
  String unsafeReason = ""; 

  bool badPh = (phValue < 6.5 || phValue > 8.5);
  bool badChlorine = (chlorineValue < 0.2 || chlorineValue > 2.0);
  bool badTurbidity = (turbidityValue > 50);
  bool badTds = (tdsValue > 500);

  if (badPh) {
    isWaterSafe = false;
    addReason(unsafeReason, "pH");
  }
  if (badChlorine) {
    isWaterSafe = false;
    addReason(unsafeReason, "Chlorine");
  }
  if (badTurbidity) {
    isWaterSafe = false;
    addReason(unsafeReason, "Turbidity");
  }
  if (badTds) {
    isWaterSafe = false;
    addReason(unsafeReason, "TDS");
  }

#if JSON_OUTPUT
  Serial.print("{\"ph\":");        Serial.print(phValue, 2);
  Serial.print(",\"chlorine\":");  Serial.print(chlorineValue, 2);
  Serial.print(",\"turbidity\":"); Serial.print(turbidityValue, 1);
  Serial.print(",\"tds\":");       Serial.print(tdsValue, 0);
  Serial.print(",\"safe\":");      Serial.print(isWaterSafe ? "true" : "false");
  Serial.print(",\"reasons\":[");
  bool first = true;
  if (badPh)        { Serial.print("\"pH\"");        first = false; }
  if (badChlorine)  { if (!first) Serial.print(","); Serial.print("\"Chlorine\"");  first = false; }
  if (badTurbidity) { if (!first) Serial.print(","); Serial.print("\"Turbidity\""); first = false; }
  if (badTds)       { if (!first) Serial.print(","); Serial.print("\"TDS\""); }
  Serial.println("]}");
#else
  Serial.println();
  Serial.print("PH: ");
  Serial.println(phValue);
  Serial.print("Chlorine: ");
  Serial.println(chlorineValue);
  Serial.print("Turbidity: ");
  Serial.println(turbidityValue);
  Serial.print("TDS: ");
  Serial.println(tdsValue);
  Serial.print("Safe Water: ");
  Serial.println(isWaterSafe ? "SAFE" : "UNSAFE");
  Serial.print("Reason: ");
  Serial.println(unsafeReason);
  Serial.println("----------");
#endif
}

void sendDataToESP() {
  String dataToSend = "";
  dataToSend += "PH=" + String(phValue, 2);
  dataToSend += "&TURB=" + String(turbidityValue, 0);
  dataToSend += "&TDS=" + String(tdsValue, 0);
  dataToSend += "&CHLOR=" + String(chlorineValue, 1);

  espSerial.println(dataToSend);

#if !JSON_OUTPUT
  Serial.print("Sending to ESP: ");
  Serial.println(dataToSend);
#endif
}