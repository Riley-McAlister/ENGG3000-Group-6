// gateway
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <WiFiUdp.h>
#define CHANNEL 1
const char *ssid = "SENSBOX1_GATEWAY_2";
const char *password = "PASSWORD";
WiFiUDP udp;
IPAddress target_IP(192, 168, 4, 2);
IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);
typedef struct
{
  uint8_t senderID;
  float distance1;
  float distance2;
} SenderData;
SenderData sender1Data;
SenderData sender2Data;
// distances[0] = GatewaySensor1
// distances[1] = GatewaySensor2
// distances[2] = Sender1Sensor1
// distances[3] = Sender1Sensor2
// distances[4] = Sender2Sensor1
// distances[5] = Sender2Sensor2
// default distances.
float distances[6] = {500.0f, 500.0f, 500.0f, 500.0f, 500.0f, 500.0f};
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int data_len)
{
  SenderData receivedData;
  if (data_len == sizeof(receivedData))
  {
    memcpy(&receivedData, data, sizeof(receivedData));
  }
  // if senderID = 1, received from sensBox2
  if (receivedData.senderID == 1)
  {
    distances[2] = receivedData.distance1;
    distances[3] = receivedData.distance2;
    Serial.println("Recieved data from sensBox2");
  }
  // if senderID = 2, received from sensBox3
  else if (receivedData.senderID == 2)
  {
    distances[4] = receivedData.distance1;
    distances[5] = receivedData.distance2;
    Serial.println("Recieved data from sensBox3");
  }
  Serial.println("SenderID: ");
  Serial.println("Distance 1: ");
  Serial.println(receivedData.distance1);
  Serial.println("Distance 2: ");
  Serial.println(receivedData.distance2);
}
void setup()
{
  Serial.begin(115200);
  Serial.println(WiFi.macAddress());
  WiFi.mode(WIFI_AP);
  WiFi.begin();
  WiFi.softAP(ssid, password);
  Serial.println(WiFi.softAPIP());
  WiFi.setSleep(false);
  // ESP_NOW config
  esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_now_init();
  esp_now_register_recv_cb(OnDataRecv);
  udp.begin(55555);
}
void loop()
{
  if (WiFi.softAPgetStationNum() > 0)
  {
    udp.beginPacket(target_IP, 55555);
    udp.write((uint8_t *)&distances, sizeof(distances));
    if (udp.endPacket())
    {
      Serial.println("UDP Packet sent successfully.");
    }
    else
    {
      Serial.println("Failed to send UDP packet.");
    }
  }
  delay(100);
}
