#include <WiFi.h>
#include <WebServer.h>

// Apne Mobile Hotspot ya WiFi ka detail dalein
const char* ssid = "YOUR_WIFI_NAME";      
const char* password = "YOUR_WIFI_PASSWORD"; 

WebServer server(80);

#define MQ_PIN 34         
#define FLAME_PIN 35      
#define TRIG_PIN 25       
#define ECHO_PIN 26       
#define BUZZER_PIN 12     

void setup() {
  Serial.begin(115200);
  
  pinMode(MQ_PIN, INPUT);
  pinMode(FLAME_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) { 
    delay(500); 
    Serial.print("."); 
  }
  
  Serial.println("\n--- DEV-X 1.0 SENSOR NODE ONLINE ---");
  Serial.print("IP ADDRESS: ");
  Serial.println(WiFi.localIP()); 

  server.on("/telemetry", handleTelemetry);
  server.begin();
}

void handleTelemetry() {
  int gasPPM = analogRead(MQ_PIN);
  
  int flameValue = analogRead(FLAME_PIN);
  int flamePercent = map(flameValue, 0, 4095, 100, 0); 
  if(flamePercent < 0) flamePercent = 0;
  
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  float distanceCm = duration * 0.034 / 2;

  String alert = "NORMAL";
  
  if(gasPPM > 1500 || flamePercent > 50 || (distanceCm > 0 && distanceCm < 50)) {
    alert = "HAZARD_DETECTED";
    digitalWrite(BUZZER_PIN, HIGH); 
  } else {
    digitalWrite(BUZZER_PIN, LOW);  
  }

  String json = "{";
  json += "\"gas\":" + String(gasPPM) + ",";
  json += "\"flame\":" + String(flamePercent) + ",";
  json += "\"distance\":" + String(distanceCm) + ",";
  json += "\"status\":\"" + alert + "\"";
  json += "}";

  server.sendHeader("Access-Control-Allow-Origin", "*"); 
  server.send(200, "application/json", json);
}

void loop() {
  server.handleClient();
}
