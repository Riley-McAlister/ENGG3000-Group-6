//gateway
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <WiFiUdp.h>
#define CHANNEL 1

const char* ssid = "SENSBOX1_GATEWAY_2";
const char* password = "PASSWORD";

WiFiUDP udp;

IPAddress target_IP(192,168,4,2);
IPAddress local_IP(192,168,4,1);
IPAddress gateway(192,168,4,1);
IPAddress subnet(255,255,255,0);

typedef struct {
   float Sens3Distance;
   float Sens4Distance;
} SensorData;
SensorData SensBox2SensorData;
SensorData def;

void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int data_len){
  memcpy(&def, data, sizeof(data));
  Serial.print("Recieved Sens3DistanceCM ");
  Serial.print(def.Sens3Distance);
  Serial.print("Recieved Sens4DistanceCM ");
  Serial.print(def.Sens4Distance);
  Serial.println(" cm");
}

void setup(){
  Serial.begin(115200);
  Serial.println(WiFi.macAddress());
  WiFi.mode(WIFI_AP);
  WiFi.begin();
  WiFi.softAP(ssid, password);
  Serial.println(WiFi.softAPIP());
  WiFi.setSleep(false);

  //ESP_NOW config
  esp_wifi_set_channel (CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_now_init();
  esp_now_register_recv_cb(OnDataRecv);  

  udp.begin(55555);

  def.Sens3Distance = 500.0f;
  def.Sens4Distance = 500.0f;
}

void loop(){
  if(WiFi.softAPgetStationNum() > 0){
  udp.beginPacket(target_IP, 55555);
  udp.write((uint8_t*) &def, sizeof(def));

  if(udp.endPacket()){
    Serial.println("UDP Packet sent successfully.");
  } else {
    Serial.println("Failed to send UDP packet.");
  }
}
  delay(100);
}

