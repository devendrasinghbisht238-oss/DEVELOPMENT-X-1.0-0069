#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Wi-Fi Credentials
const char* ssid = "YOUR_WIFI_OR_HOTSPOT_NAME";
const char* password = "YOUR_WIFI_OR_HOTSPOT_PASS";

// UiPath API Link from PDF
const char* uipathApiUrl = "https://cloud.uipath.com/8eb225f2-322e-4cba-9d5a-9a43ad1d97fa/83f9ba01-516e-4d77-b712-afb0cc702801/orchestrator_/api/services/app/QueueItems/AddQueueItem";

// Pin Definitions
#define MQ2_GAS_PIN     34    // MQ Gas Sensor
#define UV_SENSOR_PIN   35    // UV / Flame Sensor Analog Pin
#define TRIG_PIN        12    // Ultrasonic Trig
#define ECHO_PIN        13    // Ultrasonic Echo
#define BUZZER_PIN      25    // Local Alarm Buzzer

const int GAS_THRESHOLD = 400;
const int UV_THRESHOLD = 500;
unsigned long lastTriggerTime = 0;

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[SYSTEM OK] Sensor Controller Online! Local IP: " + WiFi.localIP().toString());
}

void sendUiPathEmailAlert(String alertType, int sensorVal) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(uipathApiUrl);
    http.addHeader("Content-Type", "application/json");

    DynamicJsonDocument doc(1024);
    JsonObject itemData = doc.createNestedObject("itemData");
    itemData["Name"] = "developmentX_Emergency_Queue";
    itemData["Priority"] = "High";
    
    JsonObject specificContent = itemData.createNestedObject("SpecificContent");
    specificContent["Project"] = "developmentX 1.0";
    specificContent["AlertType"] = alertType;
    specificContent["SensorValue"] = sensorVal;
    specificContent["Action"] = "SEND_AUTOMATED_EMAIL";

    String jsonPayload;
    serializeJson(doc, jsonPayload);

    int httpCode = http.POST(jsonPayload);
    Serial.printf("[UiPath Cloud] Trigger Status Code: %d\n", httpCode);
    http.end();
  }
}

long getUltrasonicDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  return (duration == 0) ? 999 : (duration * 0.034 / 2);
}

void loop() {
  int gasPpm = analogRead(MQ2_GAS_PIN);
  int uvLevel = analogRead(UV_SENSOR_PIN);
  long distanceCm = getUltrasonicDistance();

  Serial.printf("Gas: %d PPM | UV Level: %d | Distance: %ld cm\n", gasPpm, uvLevel, distanceCm);

  if ((gasPpm > GAS_THRESHOLD || uvLevel > UV_THRESHOLD) && (millis() - lastTriggerTime > 15000)) {
    digitalWrite(BUZZER_PIN, HIGH);
    Serial.println("!!! EMERGENCY: HAZARD DETECTED -> TRIGGERING UIPATH EMAIL !!!");
    
    sendUiPathEmailAlert("HAZARD_DETECTED", gasPpm);
    lastTriggerTime = millis();
    delay(1000);
    digitalWrite(BUZZER_PIN, LOW);
  }

  delay(1000);
}
