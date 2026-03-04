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
#define WATER_PUMP 15
#define LIGHT_SWITCH 17
#define MSENSE1 16
#define MSENSE2 22
#define MSENSE3 32
#define LSENSE 18

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
      mqttClient.subscribe("bedroom/esp32/#");
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

  wifiClient.setInsecure();
  mqttClient.setServer(mqtt_broker,mqtt_port);
  mqttClient.setCallback(mqttCallback);

  mqttQueue = xQueueCreate(10,sizeof(struct MQTTmessage*));
  pinMode(WATER_PUMP,OUTPUT);
  pinMode(LIGHT_SWITCH,OUTPUT);
  pinMode(MSENSE1,INPUT);
  pinMode(MSENSE2,INPUT);
  pinMode(MSENSE3,INPUT);

  xTaskCreatePinnedToCore(mqttLoop,"loop",2480,NULL,5,&mqttTaskHandle,0);
  xTaskCreatePinnedToCore(setPump,"pump",2048,NULL,4,&pumpTaskHandle,1);
  xTaskCreatePinnedToCore(setLight,"light",2048,NULL,3,&lightSwitch,1);
  xTaskCreatePinnedToCore(moistureSensor,"moisure",2048,NULL,2,&moistureTaskHandle,1);
  xTaskCreatePinnedToCore(lightSensor,"lSensor",2048,NULL,1,&lightTaskHandle,1);
  xTaskCreatePinnedToCore(automaticPlant,"auto",2048,NULL,1,&autoMode,1);


}

void mqttLoop(void *paramter){//sends the data from the queue and also loops the mqtt.loop 

  while(true){
    if(!mqttClient.connected())
      mqttReconnect();
    mqttClient.loop();

  MQTTmessage message;
  if( xQueueReceive(mqttQueue, &message, pdMS_TO_TICKS(10)) == pdTRUE){//checks if the connection of the queue is true 
      mqttClient.publish(message.topic, message.payload);//only way to publish any data to the mqtt client
  }else{//if queue is not active or not found exit code
    Serial.println("Queue not found please try again");
    exit(-1);

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
      }
    }
  }
}

void setLight(void *parameter){ //sets up the light switch and is off by default 
 uint32_t value;
  while(true){
   if(xTaskNotifyWait(0x00,ULONG_MAX,&value,portMAX_DELAY) == pdTRUE){
    if (value == 0){
      digitalWrite(LIGHT_SWITCH,LOW);
    }else if (value == 1){
      int totalTime = 0;
      uint32_t holder = 0;
      while(totalTime < 1800000){//this allows for the user to turn off the light before the 30 minute timer if they choose so
        if(xTaskNotifyWait(0x00,ULONG_MAX,&holder,portMAX_DELAY) == pdTRUE){
          if (holder == 0){
            digitalWrite(LIGHT_SWITCH,LOW);
            break;
          }

        }
          totalTime += 100;
          vTaskDelay(100 / portTICK_PERIOD_MS);
      }
    }
      digitalWrite(LIGHT_SWITCH,LOW);
      MQTTmessage msg;

      xQueueSend(mqttQueue,&msg,portMAX_DELAY);

   }
  }
}

void moistureSensor(void *parameter){//sends the moisture data to the queue and is on by default 
  while(true){
    MQTTmessage msg;
    float ms1 = analogRead(MSENSE1);
    float ms2 = analogRead(MSENSE2);
    float ms3 = analogRead(MSENSE3);
    int avg = (map(ms1,615,272,0,100) + map(ms2,615,272,0,100) + map(ms3,615,272,0,100))/3;
    snprintf(msg.payload, sizeof(msg.payload), "%d", avg);
    snprintf(msg.topic,sizeof(msg.topic),"bedroom/esp32/sensor/moisture");
    xQueueSend(mqttQueue,&msg,portMAX_DELAY);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  


}

void lightSensor(void *parameter){//sends light data to the queue and is on by default
  while(true){
    MQTTmessage msg;
    float lightSense = analogRead(LSENSE);
    int fixedValues = map(lightSense,0,1000,0, 100);
    snprintf(msg.payload, sizeof(msg.payload), "%d", fixedValues);
    snprintf(msg.topic,sizeof(msg.topic),"bedroom/esp32/sensor/light");

    snprintf(msg.payload, sizeof(msg.payload), "%d", 0);
    snprintf(msg.topic,sizeof(msg.topic),"%d","check/esp32/light");

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
