// Sender ESP
#include "Arduino.h"
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

#define CHANNEL 1
#define FILTER_SIZE 3

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

typedef struct {
  float readings[FILTER_SIZE]= { 0 };
  int count= 0;
  int nextIndex= 0;
  float lastFiltered= 0;
} DistanceFilter;

BoxPayload box;

DistanceFilter filter1;
DistanceFilter filter2;

void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
  Serial.printf("Sending Box %d: [distance1: %.2f cm, distance2: %.2f cm]", (box.id + 1), distance1, distance2);
  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.printf(" | Success\n");
  } else {
    Serial.printf(" | Failed\n");
  }
}

float medianOfThree(float a, float b, float c) {
  if (a > b) {
    float temp= a;
    a= b;
    b= temp;
  }
  if (b > c) {
    float temp= b;
    b= c;
    c= temp;
  }
  if (a > b) {
    float temp= a;
    a= b;
    b= temp;
  }
  return b;
}

float filterDistance(DistanceFilter &filter, float newReading) {
    if (newReading < 0.0f) { // Ignore invalid readings
        return filter.lastFiltered; 
    }
    filter.readings[filter.nextIndex]= newReading;
    filter.nextIndex= (filter.nextIndex + 1) % FILTER_SIZE;

    if (filter.count < FILTER_SIZE) {   
        filter.count++;
    }

    if (filter.count < FILTER_SIZE) { // Not enough readings yet, return the new reading
        filter.lastFiltered= newReading; 
        return newReading;
    }

    filter.lastFiltered= medianOfThree(
        filter.readings[0], 
        filter.readings[1], 
        filter.readings[2]
    );

    return filter.lastFiltered;
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
  float filteredDistance1= filterDistance(filter1, distance1);
  float filteredDistance2= filterDistance(filter2, distance2);

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

    box.distance[0]= filteredDistance1;
    box.distance[1]= filteredDistance2;

  // ESP-NOW
    esp_now_send(gateway.peer_addr, (uint8_t *)&box, sizeof(box));
  }
delay(1);
}