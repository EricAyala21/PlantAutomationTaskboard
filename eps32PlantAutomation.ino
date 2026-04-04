#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// Wi-Fi credentials
const char* ssid = "";
const char* password = "";

// MQTT broker
const char* mqtt_broker = "";
const int mqtt_port = 8883;
const char* mqtt_username = "";
const char* mqtt_password = "";

// Pins
#define WATER_PUMP 22
#define LIGHT_SWITCH 2
#define MSENSE2 35
#define MSENSE3 32
#define LSENSE 33

// Tasks
TaskHandle_t moistureTaskHandle = NULL;
TaskHandle_t lightTaskHandle = NULL;
TaskHandle_t lightSwitch = NULL;
TaskHandle_t pumpTaskHandle = NULL;
TaskHandle_t mqttTaskHandle = NULL;
TaskHandle_t autoMode = NULL;

// Queue for messages
QueueHandle_t mqttQueue;


// MQTT client
WiFiClientSecure wifiClient;
PubSubClient mqttClient(wifiClient);

struct MQTTmessage{
  char topic[50];
  char payload[50];
};

void mqttReconnect(){
  while(!mqttClient.connected()){
    Serial.println("Reconnecting MQTT...");
    String clientId = "ESP32Client-" +String(random(0xffff),HEX);
    if(mqttClient.connect(clientId.c_str(), mqtt_username, mqtt_password)){
      Serial.println("MQTT connected");
      mqttClient.subscribe("bedroom/esp32/waterPump");
      mqttClient.subscribe("bedroom/esp32/lightSwitch");

      mqttClient.subscribe("check/esp32/light");
      mqttClient.subscribe("check/esp32/autoMode");

    }else{
      Serial.print("MQTT failed rc=");
      Serial.println(mqttClient.state());
      delay(5000);
    }
  }
}
void mqttCallback(char* topic, byte* payload, unsigned int length){
  if(length >= 50)
    length = 49; // protects the size of the queue
  char msg[50];
  
  memcpy(msg,payload,length);
  msg[length] = '\0';
  uint32_t value = static_cast<uint32_t>(atoi(msg));
  //checks to see which topic it is in to wake up each function when needed 
  
  if(strcmp(topic,"bedroom/esp32/waterPump") == 0){

    xTaskNotify(pumpTaskHandle,value,eSetValueWithOverwrite);
  }else if(strcmp(topic,"bedroom/esp32/lightSwitch") == 0){
    xTaskNotify(lightSwitch,value,eSetValueWithOverwrite);
  }else if(strcmp(topic,"check/esp32/autoMode") == 0){
    xTaskNotify(autoMode,value,eSetValueWithOverwrite);
  }
}
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  WiFi.begin(ssid,password);
  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(500);

  }
  Serial.println("\nWiFi connected");
  mqttQueue = xQueueCreate(5,sizeof(struct MQTTmessage));

  wifiClient.setInsecure();
  mqttClient.setServer(mqtt_broker,mqtt_port);
  mqttClient.setCallback(mqttCallback);

  pinMode(WATER_PUMP,OUTPUT);
  pinMode(LIGHT_SWITCH,OUTPUT);
  pinMode(MSENSE2,INPUT);
  pinMode(MSENSE3,INPUT);

  xTaskCreatePinnedToCore(mqttLoop,"loop",8192,NULL,1,&mqttTaskHandle,0);
  xTaskCreatePinnedToCore(setPump,"pump",8192,NULL,5,&pumpTaskHandle,1);
  xTaskCreatePinnedToCore(setLight,"light",8192,NULL,4,&lightSwitch,1);
  xTaskCreatePinnedToCore(moistureSensor,"moisure",8192,NULL,3,&moistureTaskHandle,1);
  xTaskCreatePinnedToCore(lightSensor,"lSensor",8192,NULL,2,&lightTaskHandle,1);
  xTaskCreatePinnedToCore(automaticPlant,"auto",8192,NULL,6,&autoMode,1);


}

void mqttLoop(void *paramter){//sends the data from the queue and also loops the mqtt.loop 

  while(true){
    if(!mqttClient.connected())
      mqttReconnect();
    mqttClient.loop();

  MQTTmessage message;
  if( xQueueReceive(mqttQueue, &message, pdMS_TO_TICKS(10)) == pdTRUE){//checks if the connection of the queue is true 
      mqttClient.publish(message.topic, message.payload);//only way to publish any data to the mqtt client
  }
  vTaskDelay(10/portTICK_PERIOD_MS); //check for any new messages every 10 ms
  }
}

void setPump(void *parameter){ //sets up the pump is asleep by default 
 uint32_t value;
  while(true){
    if(xTaskNotifyWait(0x00,ULONG_MAX,&value,portMAX_DELAY) == pdTRUE){
      if(value == 0 ){
        digitalWrite(WATER_PUMP,LOW);
      }else if (value == 1){
        digitalWrite(WATER_PUMP,HIGH);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        Serial.println("waterpump is on");
      }
    }
      digitalWrite(WATER_PUMP,LOW);

  }
}

void setLight(void *parameter){ //sets up the light switch and is off by default 
 uint32_t value;
  while(true){
   if(xTaskNotifyWait(0x00,ULONG_MAX,&value,portMAX_DELAY) == pdTRUE){
    if (value == 1){
      Serial.println("Light is on");
      int totalTime = 0;
      uint32_t holder = 0;
      digitalWrite(LIGHT_SWITCH,HIGH);
      Serial.println("Signal High");

      while(totalTime < 10000){//this allows for the user to turn off the light before the 30 minute timer if they choose so
        if(xTaskNotifyWait(0x00,ULONG_MAX,&holder,0) == pdTRUE){
          if (holder == 0){
            digitalWrite(LIGHT_SWITCH,LOW);
            break;
          }

        }
          totalTime += 100;
          vTaskDelay(100 / portTICK_PERIOD_MS);
      }
      digitalWrite(LIGHT_SWITCH,LOW);

    }else if( value == 0){
      digitalWrite(LIGHT_SWITCH,LOW);
      Serial.println("Light is LOW");

    }
      MQTTmessage msg;
      snprintf(msg.payload, sizeof(msg.payload), "%d", 0);
      snprintf(msg.topic,sizeof(msg.topic),"check/esp32/light");
      xQueueSend(mqttQueue,&msg,portMAX_DELAY);

   }
  }
}

void moistureSensor(void *parameter){//sends the moisture data to the queue and is on by default 
  while(true){
    MQTTmessage msg;
    int avg = moisterData();
    snprintf(msg.payload, sizeof(msg.payload), "%d", 100 - avg);
    snprintf(msg.topic,sizeof(msg.topic),"bedroom/esp32/sensor/moisture");
    xQueueSend(mqttQueue,&msg,portMAX_DELAY);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  


}

float moisterData(){
  float ms2 = analogRead(MSENSE2);
  float ms3 = analogRead(MSENSE3);
  int avg = (map(ms2,975,2672,0,100) +map(ms3,902,2707,0,100))/2;
  return avg;
}

void lightSensor(void *parameter){//sends light data to the queue and is on by default
  while(true){
    MQTTmessage msg;
    int lightSense = analogRead(LSENSE);
    int fixedValues = map(lightSense,0,2432,0, 100);
    snprintf(msg.payload, sizeof(msg.payload), "%d", fixedValues);
    snprintf(msg.topic,sizeof(msg.topic),"bedroom/esp32/sensor/light");
    Serial.println(fixedValues);
    xQueueSend(mqttQueue,&msg,portMAX_DELAY);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void automaticPlant(void *paramter){//will control when the light and pump will be activated based on the data from the sensors
 uint32_t value;
  while(true){
    if(xTaskNotifyWait(0x00,ULONG_MAX,&value,portMAX_DELAY) == pdTRUE){
      
    }

  }
}


void loop() {

}
