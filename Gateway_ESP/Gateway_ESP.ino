// Gateway ESP
#include "Arduino.h"
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <WiFiUdp.h>

#define CHANNEL 1
#define FILTER_SIZE 3
#define BOX_COUNT 3
#define SENSOR_COUNT 2
#define PLAYFIELD_X 150.0f
#define PLAYFIELD_Y 200.0f

// Sensor cfg
const int trigPin1= 26;
const int echoPin1= 27;
const int trigPin2= 32;
const int echoPin2= 33;

float duration1;
float distance1;
float duration2;
float distance2;

const char* ssid= "ES3-AM-06-GATEWAY";
const char* password= "PASSWORD";
WiFiUDP udp;

// IP Addresses
IPAddress target_IP(192,168,1,2);
IPAddress local_IP(192,168,1,1);
IPAddress subnet(255,255,255,0);
IPAddress gateway(192,168,1,1);

int clientAssigned= false;

unsigned long prevMs= 0; // in milliseconds (ms)
const long interval= 100;

typedef struct {
  uint8_t id;
  float distance[2];
} BoxPayload;
typedef struct {
  BoxPayload data[3];
} DataPayload;
typedef struct {
  float loc[2];
} PlayerLoc;

DataPayload dp;
PlayerLoc pl;

typedef struct {
  float readings[FILTER_SIZE] = {0};
  int count = 0;
  int nextIndex = 0;
  float lastFiltered = 0;
} DistanceFilter;

DistanceFilter filter1;
DistanceFilter filter2;


void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
      clientAssigned= true;
      Serial.printf("UDP broadcast started\n");
      break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      clientAssigned= false;
      Serial.printf("UDP broadcast paused\n");
      break;
    default:
      break;
  }
}

void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int data_len) {
  if(data_len == sizeof(BoxPayload)) {
    BoxPayload bp;
    memcpy(&bp, data, sizeof(BoxPayload));

    if(bp.id < BOX_COUNT) {
      dp.data[bp.id].distance[0]= bp.distance[0];
      dp.data[bp.id].distance[1]= bp.distance[1];
    }
    // Serial.printf("Received Box %d: [distance1: %.2f cm, distance2: %.2f cm]\n", (bp.id + 1), bp.distance[0], bp.distance[1]);
  }
}

float medianOfThree(float a, float b, float c) {
  if(a > b){
    float temp = a;
    a = b;
    b = temp;
  }
  if(b > c){
    float temp = b;
    b = c;
    c = temp;
  }
  if(a > b){
    float temp = a;
    a = b;
    b = temp;
  }
  return b;
}

float filterDistance(DistanceFilter &filter, float newReading){
    if( newReading <= 0.0f){
        return filter.lastFiltered; // Ignore invalid readings
    }
    filter.readings[filter.nextIndex] = newReading;
    filter.nextIndex = (filter.nextIndex + 1) % FILTER_SIZE;

    if (filter.count < FILTER_SIZE) {   
        filter.count++;
    }

    if(filter.count < FILTER_SIZE){
        filter.lastFiltered = newReading; // Not enough readings yet, return the new reading
        return newReading;
    }

    filter.lastFiltered = medianOfThree(
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

  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleep(false);

  WiFi.onEvent(WiFiEvent);

  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password);

  if (esp_now_init() != ESP_OK) {
    Serial.printf("Error initializing ESP-NOW\n");
    return;
  }

  esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_now_register_recv_cb(OnDataRecv);

  // Port number for broadcasting UDP packets
  udp.begin(55555);
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
    dp.data[0].distance[0]= filteredDistance1;
    dp.data[0].distance[1]= filteredDistance2;

    if (clientAssigned) {
      Serial.printf("Sending DataPayload: [%.2f, %.2f, %.2f, %.2f, %.2f, %.2f]",  
      dp.data[0].distance[0], dp.data[0].distance[1], 
      dp.data[1].distance[0], dp.data[1].distance[1], 
      dp.data[2].distance[0], dp.data[2].distance[1]);

      udp.beginPacket(target_IP, 55555);
      udp.write((uint8_t*) &dp, sizeof(dp));

      if (udp.endPacket()) {
        Serial.println(" | Success");
      } else {
        Serial.println(" | Failed");
      }
    }
  }
  delay(1);
}