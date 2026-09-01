//Sender
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#define CHANNEL 1
//Ultrasonic sensors
const int Sens3trigPin = 26;
const int Sens3echoPin = 27;
const int Sens4trigPin = 32;
const int Sens4echoPin = 33;
float Sens3Duration;
float Sens3DistanceCM;
float Sens4Duration;
float Sens4DistanceCM;

//ESP Now send info 
uint8_t receiverAddress[] = {0xB0, 0xA7, 0x32, 0xB6, 0xA5, 0xAC};
esp_now_peer_info_t SensBox1;
typedef struct {
   float Sens3Distance;
   float Sens4Distance;
} SensorData;
SensorData SensBox2SensorData;
void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status){
  Serial.print ("Sent Sens3 Distance ");
  Serial.print(SensBox2SensorData.Sens3Distance);
  Serial.print ("cm | Status: ");
  if (status == ESP_NOW_SEND_SUCCESS){
    Serial.print("Success | ");
  } else {
    Serial.print("Failed | ");
  }
  Serial.print ("Sent Sens4 Distance ");
  Serial.print(SensBox2SensorData.Sens4Distance);
  Serial.print ("cm | Status: ");
  if (status == ESP_NOW_SEND_SUCCESS){
    Serial.println("Success");
  } else {
    Serial.println("Failed");
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  
  //Ultrasonic sensor 3
  pinMode(Sens3trigPin, OUTPUT);
  pinMode(Sens3echoPin, INPUT);
  
  //Ultrasonic sensor 4
  pinMode(Sens4trigPin, OUTPUT);
  pinMode(Sens4echoPin, INPUT);
  //ESP Now
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
}
void loop() {
  // put your main code here, to run repeatedly:
  //ultrasonic sensor 3
  digitalWrite(Sens3trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(Sens3trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(Sens3trigPin, LOW);
  Sens3Duration = pulseIn(Sens3echoPin, HIGH);
  Sens3DistanceCM = (Sens3Duration * 0.034) / 2;
  SensBox2SensorData.Sens3Distance = Sens3DistanceCM;
  
  //ultrasonic sensor 4
  digitalWrite(Sens4trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(Sens4trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(Sens4trigPin, LOW);
  Sens4Duration = pulseIn(Sens4echoPin, HIGH);
  Sens4DistanceCM = (Sens4Duration * 0.034) / 2;
  SensBox2SensorData.Sens4Distance = Sens4DistanceCM;
  //esp now
  esp_now_send(SensBox1.peer_addr, (uint8_t *)&SensBox2SensorData, sizeof(SensBox2SensorData));
  delay(200);
}



