// Sender ESP
#include "Arduino.h"
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

#define CHANNEL 1

// Sensor config
const int trigPin1= 26;
const int echoPin1= 27;
const int trigPin2= 32;
const int echoPin2= 33;

float duration1;
float distance1;
float duration2;
float distance2;

// ESP-NOW send info
uint8_t gatewayAddress[]= {0xB0, 0xA7, 0x32, 0xB6, 0xA5, 0xAC}; // Gateway should always be Box 1 (uint8_t hexadecimal for ESP-NOW)
String senderAddress[2]= { // String for C++ comparisons
  "00:70:07:7C:8A:58", // Box 2
  "04:B2:47:FA:77:DC" // Box 3
};
esp_now_peer_info_t gateway;

unsigned long prevMs= 0;
const long interval= 100;

typedef struct {
  uint8_t id;
  float distance[2];
} BoxPayload;

BoxPayload box;

void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
  Serial.printf("Sending Box %d: [distance1: %.2f cm, distance2: %.2f cm]", (box.id + 1), distance1, distance2);
  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.printf(" | Success\n");
  } else {
    Serial.printf(" | Failed\n");
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);

  delay(500);

  WiFi.mode(WIFI_STA);

  esp_now_init();
  esp_now_register_send_cb(OnDataSent);

  memcpy(gateway.peer_addr, gatewayAddress, sizeof(gatewayAddress));
  gateway.channel= CHANNEL;  
  gateway.ifidx= WIFI_IF_STA;
  gateway.encrypt= false;

  if (esp_now_add_peer(&gateway) != ESP_OK) {
    Serial.printf("Failed to add peer\n");
  }

  String macAdd= WiFi.macAddress();
  if (macAdd == senderAddress[0]) {
    box.id= 1;
  } else {
    box.id= 2; 
  }
}

void loop() {
  unsigned long currMs= millis();

  if (currMs - prevMs >= interval) {
    prevMs= currMs;
    digitalWrite(trigPin1, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin1, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin1, LOW);
    duration1= pulseIn(echoPin1, HIGH, 15000);
    distance1= duration1 * 0.017;

    digitalWrite(trigPin2, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin2, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin2, LOW);
    duration2= pulseIn(echoPin2, HIGH, 15000);
    distance2= duration2 * 0.017;

    box.distance[0]= distance1;
    box.distance[1]= distance2;

  // ESP-NOW
    esp_now_send(gateway.peer_addr, (uint8_t *)&box, sizeof(box));
  }
delay(1);
}