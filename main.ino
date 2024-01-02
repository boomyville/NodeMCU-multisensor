#include <Wire.h>
#include "DHT.h"
#include "MQ135.h" 

/*
 * https://hackaday.io/project/3475-sniffing-trinket/log/12363-mq135-arduino-library
 */

#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>

#define WIFI_SSID *****
#define WIFI_PASSWORD *****

// Mosquitto MQTT Broker
#define MQTT_HOST IPAddress(***, ***, ***, ***)
// For a cloud MQTT broker, type the domain name
//#define MQTT_HOST "example.com"
#define MQTT_PORT 1883

// Temperature MQTT Topics
#define MQTT_PUB_TEMP "nodemcu/dht/temperature"
#define MQTT_PUB_HUM "nodemcu/dht/humidity"
#define MQTT_PUB_RZERO "nodemcu/mq135/rzero"
#define MQTT_PUB_AIRQUALITY "nodemcu/mq135/airquality"

// Microphone MQTT Topics
#define MQTT_PUB_SOUND "nodemcu/ky037/sound"


// Digital pin connected to the DHT sensor
#define DHTPIN 5  //GPIO4 or D4
uint8_t DHTPin = D5; 

// Microphone pin
int sound_digital = D7;
int sound_value;
bool sound;

// Uncomment whatever DHT sensor type you're using
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)   

// Initialize DHT sensor
DHT dht(DHTPin, DHTTYPE);

// Initialize MQ-135
MQ135 gasSensor = MQ135(0); //Assumes A0 is the analogue pin 0

// Variables to hold sensor readings
float temp;
float hum;
float airquality = 0;
float rzerovalue = 0;
int mq135 = A0; //MQ-135 data is an analogue value that is acquired via the A0 port of the nodeMCU

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

unsigned long previousMillis = 0;   // Stores last time temperature was published
const long interval = 10000;        // Interval at which to publish sensor readings

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("Connected to Wi-Fi.");
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi.");
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

/*void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}*/

void onMqttPublish(uint16_t packetId) {
  Serial.print("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void setup() {
  Serial.begin(115200);
  Serial.println("START");

  dht.begin();

  pinMode(sound_digital, INPUT);  
  sound_value = 0;
sound = false;
  
  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
  
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  //mqttClient.onSubscribe(onMqttSubscribe);
  //mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  // If your broker requires authentication (username and password), set them below
  mqttClient.setCredentials("boomy", "thai1ma2");
  
  connectToWifi();
}

void loop() {
  
  //Microphone
    sound_value = digitalRead(sound_digital);
if(sound_value == 1 && sound == false) {
  sound = true;
} 
delay(1);
  
  unsigned long currentMillis = millis();
  // Every X number of seconds (interval is defined earlier) 
  // it publishes a new MQTT message
  if (currentMillis - previousMillis >= interval) {
    // Save the last time a new reading was published
    previousMillis = currentMillis;
    // New DHT sensor readings
    hum = dht.readHumidity();
    // Read temperature as Celsius (the default)
    temp = -2 + dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    //temp = dht.readTemperature(true);
    // Read MQ-135 data 
    airquality = gasSensor.getCorrectedPPM(temp, hum);
    rzerovalue = gasSensor.getCorrectedRZero(temp, hum);
    
    // Check if any reads failed and exit early (to try again).
    if (isnan(temp) || isnan(hum)) {
      Serial.println(F("Failed to read from DHT sensor!"));
      return;
    } else if (isnan(mq135)) {
      Serial.println(F("Failed to read from MQ-135 sensor!"));
      return;
    }
    
    // Publish an MQTT message on topic /dht/temperature
    uint16_t packetIdPub1 = mqttClient.publish(MQTT_PUB_TEMP, 1, true, String(temp).c_str());                            
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i", MQTT_PUB_TEMP, packetIdPub1);
    Serial.printf("Message: %.2f \n", temp);

    // Publish an MQTT message on topic /dht/humidity
    uint16_t packetIdPub2 = mqttClient.publish(MQTT_PUB_HUM, 1, true, String(hum).c_str());                            
    Serial.printf("Publishing on topic %s at QoS 1, packetId %i: ", MQTT_PUB_HUM, packetIdPub2);
    Serial.printf("Message: %.2f \n", hum);

    // Publish an MQTT message on topic /mq135/rzero
    uint16_t packetIdPub3 = mqttClient.publish(MQTT_PUB_RZERO, 1, true, String(rzerovalue).c_str());                            
    Serial.printf("Publishing on topic %s at QoS 1, packetId %i: ", MQTT_PUB_RZERO, packetIdPub3);
    Serial.printf("Message: %.2f \n", rzerovalue);
    
    // Publish an MQTT message on topic /mq135/airquality
    uint16_t packetIdPub4 = mqttClient.publish(MQTT_PUB_AIRQUALITY, 1, true, String(airquality).c_str());                            
    Serial.printf("Publishing on topic %s at QoS 1, packetId %i: ", MQTT_PUB_AIRQUALITY, packetIdPub4);
    Serial.printf("Message: %.2f \n", airquality);   

   // Publish an MQTT message on topic /ky037/alert
    //uint16_t packetIdPub5 = mqttClient.publish(MQTT_PUB_SOUND, 1, true, sound ? "1" : "0");                            
    if(sound) {
    uint16_t packetIdPub5 = mqttClient.publish(MQTT_PUB_SOUND, 1, true, "ON");                            
    Serial.printf("Publishing on topic %s at QoS 1, packetId %i: ", MQTT_PUB_SOUND, packetIdPub5);
     
    sound = false;
 } else {
uint16_t packetIdPub5 = mqttClient.publish(MQTT_PUB_SOUND, 1, true, "OFF");                            
    Serial.printf("Publishing on topic %s at QoS 1, packetId %i: ", MQTT_PUB_SOUND, packetIdPub5);
    }
      
    }

    
  }
