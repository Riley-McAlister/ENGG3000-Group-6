// Sender
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#define CHANNEL 1
// Ultrasonic sensors
const int Sens1trigPin = 26;
const int Sens1echoPin = 27;
const int Sens2trigPin = 32;
const int Sens2echoPin = 33;
float Sens1Duration;
float Sens1DistanceCM;
float Sens2Duration;
float Sens2DistanceCM;
// ESP Now send info
uint8_t receiverAddress[] = {0xB0, 0xA7, 0x32, 0xB6, 0xA5, 0xAC};
esp_now_peer_info_t SensBox1;
typedef struct
{
  uint8_t senderID;
  float distance1;
  float distance2;
} SenderData;
SenderData sender1Data;
SenderData sender2Data;

void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status)
{
  Serial.print("Sent Distance1 ");
  Serial.print(sender1Data.distance1);
  Serial.print("cm | Status: ");
  if (status == ESP_NOW_SEND_SUCCESS)
  {
    Serial.print("Success | ");
  }
  else
  {
    Serial.print("Failed | ");
  }
  Serial.print("Sent Distance2 ");
  Serial.print(sender1Data.distance2);
  Serial.print("cm | Status: ");
  if (status == ESP_NOW_SEND_SUCCESS)
  {
    Serial.println("Success");
  }
  else
  {
    Serial.println("Failed");
  }
}
void setup()
{
  // put your setup code here, to run once:
  sender1Data.distance1 = Sens1DistanceCM;
  sender1Data.distance2 = Sens2DistanceCM;
  Serial.begin(115200);

  // Ultrasonic sensor 3
  pinMode(Sens1trigPin, OUTPUT);
  pinMode(Sens1echoPin, INPUT);

  // Ultrasonic sensor 4
  pinMode(Sens2trigPin, OUTPUT);
  pinMode(Sens2echoPin, INPUT);
  // ESP Now
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  Serial.print("My MAC address: ");
  Serial.println(WiFi.macAddress());

  esp_now_init();
  esp_now_register_send_cb(OnDataSent);
  memcpy(SensBox1.peer_addr, receiverAddress, 6);
  esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE);
  SensBox1.encrypt = false;
  esp_now_add_peer(&SensBox1);

  sender1Data.senderID = 1;
}
void loop()
{
  // put your main code here, to run repeatedly:
  // ultrasonic sensor 3
  digitalWrite(Sens1trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(Sens1trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(Sens1trigPin, LOW);
  Sens1Duration = pulseIn(Sens1echoPin, HIGH);
  Sens1DistanceCM = (Sens1Duration * 0.034) / 2;

  // ultrasonic sensor 4
  digitalWrite(Sens2trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(Sens2trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(Sens2trigPin, LOW);
  Sens2Duration = pulseIn(Sens2echoPin, HIGH);
  Sens2DistanceCM = (Sens2Duration * 0.034) / 2;

  sender1Data.senderID = 1;
  sender1Data.distance1 = Sens1DistanceCM;
  sender1Data.distance2 = Sens2DistanceCM;
  // esp now
  esp_now_send(SensBox1.peer_addr, (uint8_t *)&sender1Data, sizeof(sender1Data));
  delay(200);
}
