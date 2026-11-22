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
#include <math.h> 

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

void setup() {
  Serial.begin(9600);
  
  espSerial.begin(9600); 
  
  Serial.println("Water Monitor - ESP8266 Mode");
  Serial.println("Initializing...");
  delay(2000);
  Serial.println("System Ready.");
}

void loop() {
  readSensors();

  analyzeAndDebug();

  sendDataToESP();

  delay(100); 
}

void readSensors() {
  int phRaw = analogRead(PH_PIN);
  float phVoltage = phRaw * (5.0 / 1023.0); 
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

void analyzeAndDebug() {
  bool isWaterSafe = true;
  String unsafeReason = ""; 

  if (phValue < 6.5 || phValue > 8.5) {
    isWaterSafe = false;
    unsafeReason = "pH";
  }
  if (isWaterSafe && (chlorineValue < 0.2 || chlorineValue > 2.0)) {
    isWaterSafe = false;
    unsafeReason = "Chlorine";
  }
  if (isWaterSafe && (turbidityValue > 50)) {
    isWaterSafe = false;
    unsafeReason = "Turbidity";
  }
  if (isWaterSafe && (tdsValue > 500)) {
    isWaterSafe = false;
    unsafeReason = "TDS";
  }

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

}

void sendDataToESP() {
  String dataToSend = "";
  dataToSend += "PH=" + String(phValue, 2);
  dataToSend += "&TURB=" + String(turbidityValue, 0);
  dataToSend += "&TDS=" + String(tdsValue, 0);
  dataToSend += "&CHLOR=" + String(chlorineValue, 1);

  espSerial.println(dataToSend);

  Serial.print("Sending to ESP: ");
  Serial.println(dataToSend);
}