// Gateway ESP
#include "Arduino.h"
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <WiFiUdp.h>

#define CHANNEL 1
#define BOX_COUNT 3
#define SENSOR_COUNT 2
#define PLAYFIELD_X 150.0f
#define PLAYFIELD_Y 200.0f

const char* ssid= "ES3-AM-06-GATEWAY";
const char* password= "PASSWORD";
WiFiUDP udp;

// IP Addresses
IPAddress target_IP(192,168,1,2);
IPAddress local_IP(192,168,1,1);
IPAddress subnet(255,255,255,0);
IPAddress gateway(192,168,1,1);

int clientAssigned= false;

// Anchor sensor's physical locations for calculations (in cm) 
// Center point of the sensors are 11mm from the center of the box
const float SensorPos[BOX_COUNT * SENSOR_COUNT][2]= {
  {36.4f, 0.0f},
  {38.6f, 0.0f}, // Box 1 centred at 37.5cm
  {73.9f, 0.0f},
  {76.1f, 0.0f}, // Box 2 centred at 75cm
  {111.4f, 0.0f},
  {113.6f, 0.0f} // Box 3 centred at 112.5cm
};

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
    Serial.printf("Received Box %d: [distance1: %.2f cm, distance2: %.2f cm]\n", (bp.id + 1), bp.distance[0], bp.distance[1]);
  }
}

 // Multilateration algorithm supporting 2 to 6 sensors with Least Squares & Circle Intersection
void calculatePlayerLoc() {
  int valid_count= 0;
  for (int i= 0; i < BOX_COUNT; i++) {
    for (int j= 0; j < SENSOR_COUNT; j++) {
        if (dp.data[i].distance[j] > 0.0f) {
            valid_count++;
        }
      }
    }

  if (valid_count < 2) {
    return; // Need at least 2 sensors
  }

  float best_x= 0.0f;
  float best_y= 0.0f;
  float min_error= 1e9f; // Start with a very high error

  // Playfield boundaries (in cm): X from 0 to 150, Y from (0) 60 to 200
  // Step size of 1.0 cm
  for (float x= 0.0f; x <= 150.0f; x+= 1.0f) {
    for (float y= 60.0f; y <= 200.0f; y+= 1.0f) { // 60cm deadzone
      float total_error= 0.0f;
      int active_comparisons= 0;

      for (int i= 0; i < BOX_COUNT; i++) {
        for (int j= 0; j < SENSOR_COUNT; j++) {
            if (dp.data[i].distance[j] > 0.0f) {
              int sensor_index= (i * SENSOR_COUNT) + j;
              float dx= x - SensorPos[sensor_index][0];
              float dy= y - SensorPos[sensor_index][1];

              float pred_dist_sq= (dx * dx) + (dy * dy);
              float target_dist= dp.data[i].distance[j];
              float target_dist_sq= target_dist * target_dist;
              float diff= pred_dist_sq - target_dist_sq;

              total_error+= diff * diff;
              active_comparisons++;
              }
            }
          }

      if (active_comparisons > 0) {
        float avg_error= total_error / active_comparisons;
        if (avg_error < min_error) {
          min_error= avg_error;
          best_x= x;
          best_y= y;
        }
      }
    }
    yield();
  }
  // Update player location struct with the converted best grid match
  pl.loc[0]= best_x;
  pl.loc[1]= best_y;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

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

  // Port number for receiving UDP packets
  udp.begin(55555);
}

void loop() {
  if (clientAssigned) {
      calculatePlayerLoc();
      Serial.printf("Sending PlayerLoc [X: %.2f, Y: %.2f]", pl.loc[0], pl.loc[1]);

      udp.beginPacket(target_IP, 55555);
      udp.write((uint8_t*) &pl, sizeof(pl));

      if (udp.endPacket()) {
        Serial.printf(" | Success\n");
      } else {
        Serial.printf(" | Failed\n");
      }
    }
  delay(250);
}