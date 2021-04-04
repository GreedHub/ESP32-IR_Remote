#include "IRrecv.h"
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <ArduinoJson.h>
#include <IRsend.h>
#include "ir_Electra.h"
#include <MQTTClient.h>
#include "WiFi.h"
#include "secrets.h"
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <IRutils.h>


WiFiClient net = WiFiClient();
MQTTClient client = MQTTClient(256);
 
int RECV_PIN = 36; //an IR detector connected to D4
const uint16_t kIrLed = 4;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).

IRsend irsend(kIrLed);  // Set the GPIO to be used to sending the message.

const unsigned char pwr_off[99] = {2994, 1776,  416, 1132,  414, 1134,  414, 412,  414, 412,  388, 438,  442, 1106,  414, 412,  412, 428,  386, 1160,  414, 1132,  414, 412,  386, 1160,  414, 412,  388, 438,  414, 1132,  414, 1148,  442, 384,  414, 1134,  414, 1132,  414, 412,  386, 440,  414, 1132,  388, 438,  386, 454,  414, 1134,  388, 438,  414, 410,  414, 410,  388, 438,  386, 440,  414, 412,  386, 452,  414, 412,  416, 410,  414, 412,  414, 410,  386, 438,  442, 382,  414, 412,  414, 426,  416, 410,  414, 410,  414, 412,  414, 412,  388, 438,  414, 1134,  414, 412,  414, 426,  416};
uint16_t pwr_on[227] = {2936, 1836,  356, 1192,  356, 1192,  356, 470,  354, 470,  356, 470,  356, 1192,  356, 470,  356, 484,  356, 1192,  356, 1192,  356, 470,  356, 1192,  356, 470,  356, 470,  356, 1192,  356, 1206,  356, 470,  356, 1192,  356, 1192,  356, 468,  356, 470,  356, 1192,  356, 470,  354, 484,  356, 1192,  356, 470,  356, 470,  356, 470,  354, 472,  354, 470,  354, 470,  356, 484,  356, 470,  356, 470,  354, 470,  356, 470,  354, 470,  354, 470,  358, 468,  356, 484,  356, 472,  354, 470,  354, 1192,  354, 470,  356, 470,  354, 1192,  354, 472,  354, 484,  354, 1192,  356, 1192,  354, 470,  356, 470,  354, 472,  354, 472,  354, 470,  356, 484,  354, 472,  354, 470,  354, 470,  356, 1192,  354, 470,  354, 472,  354, 448,  378, 484,  354, 472,  354, 470,  354, 472,  354, 472,  354, 470,  356, 470,  354, 1194,  354, 484,  354, 472,  354, 472,  354, 472,  354, 470,  356, 470,  354, 470,  354, 472,  354, 484,  354, 472,  354, 470,  354, 446,  378, 472,  354, 470,  354, 472,  354, 470,  356, 484,  354, 472,  354, 472,  354, 472,  354, 472,  354, 470,  354, 470,  354, 470,  354, 484,  356, 470,  356, 470,  354, 446,  378, 472,  354, 470,  354, 446,  378, 472,  354, 484,  358, 466,  354, 472,  354, 1192,  354, 472,  354, 472,  354, 470,  354, 472,  354, 1208,  354};

//IRsend irsend(led); // Set the GPIO to be used to sending the message.
IRrecv irrecv(RECV_PIN);
decode_results results;

void connectMQTT(){

  WiFiManager wifiManager;

  if(!wifiManager.autoConnect(WIFI_SSID,WIFI_PASSWORD)) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(1000);
  } 

  Serial.println("Connected to Wi-Fi");

  // Connect to the MQTT broker
  client.begin(MQTT_ENDPOINT, MQTT_PORT, net);

  // Create a message handler
  client.onMessage(messageHandler);

  Serial.println("Connecting to MQTT");

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if(!client.connected()){
    Serial.println("MQTT Timeout!");
    return;
  }
  Serial.println("MQTT Connected!");
  Serial.println("Subscribing to irtopic");

  // Subscribe to topics
  client.subscribe(IR_SEND_TOPIC);

  Serial.println("");
  Serial.println("Success!");
}

void messageHandler(String &topic, String &payload) {

   //Serial.println("incoming: " + topic + " - " + payload);
    StaticJsonDocument<200> doc;
   
    deserializeJson(doc, payload);
   
   char _topic[topic.length()+1];
   topic.toCharArray(_topic,topic.length()+1);

  if(strcmp(_topic,(char*)IR_SEND_TOPIC)==0){


    
    const char* type = doc["type"];//body has to be {"type":"NEC","code":"123AC123","bytes":16}
    const char* code = doc["code"];
    int bytes = (int) doc["bytes"];
    int address = (int) doc["address"];

    sendIrCode(type,  code, bytes, address);
  }

}

void dump(decode_results *results, String rawData) {
  // Dumps out the decode_results structure.
  // Call this after IRrecv::decode()
  DynamicJsonDocument jsonPayload(1024); 

  if(results->decode_type == ELECTRA_AC){
    jsonPayload["encoding"] = "ELECTRA_ACx";
  }
  else if (results->decode_type == UNKNOWN) {
    jsonPayload["encoding"] = "Unknown";
  }
  else if (results->decode_type == NEC) {
    jsonPayload["encoding"] = "NEC";
  }
  else if (results->decode_type == SONY) {
    jsonPayload["encoding"] = "SONY";
  }
  else if (results->decode_type == RC5) {
    jsonPayload["encoding"] = "RC5";
  Serial.print("Decoded RC5: ");
  }
  else if (results->decode_type == RC6) {
    jsonPayload["encoding"] = "RC6";  
  }
  else if (results->decode_type == PANASONIC) {
    jsonPayload["encoding"] = "PANASONIC";  
    
  }
  else if (results->decode_type == LG) {
    jsonPayload["encoding"] = "LG";
  }
  else if (results->decode_type == JVC) {
    jsonPayload["encoding"] = "JVC";
  }
  else if (results->decode_type == AIWA_RC_T501) {
    jsonPayload["encoding"] = "AIWA_RC_T501";
  }
  else if (results->decode_type == WHYNTER) {
    jsonPayload["encoding"] = "WHYNTER";
  }

  //jsonPayload["raw"] = results->rawbuf; not yet implemented
  jsonPayload["rawLength"] = String(results->rawlen,DEC);
  jsonPayload["hex"] =  String((uint32_t) results->value, HEX);
  jsonPayload["bin"] =  String((uint32_t) results->value, BIN);
  jsonPayload["bits"] = String((uint32_t) results->bits, DEC);
  jsonPayload["address"] = String((uint32_t) results->address, HEX);  

  String payload;

  serializeJson(jsonPayload, payload);

  client.publish(IR_READ_TOPIC,payload);
}

void setup() {
  Serial.begin(115200);
  irrecv.enableIRIn();
  irsend.begin();
  connectMQTT();

  #if ESP8266
    Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
  #else  // ESP8266
    Serial.begin(115200, SERIAL_8N1);
  #endif  // ESP8266
}

void sendIrCode(const char* _type,const char* _code, int _bytes,int _address){


    if(strcmp(_type,"NEC") == 0){

      unsigned long code = strtoul(_code,NULL,16);
      Serial.print("IR_SEND | NEC ");
      Serial.print(code,HEX);
      Serial.print(" - length:  ");
      Serial.print(_bytes,DEC);
      Serial.println(" bits");
      irsend.sendNEC(code, _bytes);
    }else if(strcmp(_type,"Panasonic") == 0){
      unsigned long long code = strtoull(_code,NULL,16);
      Serial.print("IR_SEND | Panasonic ");
      Serial.print(code,HEX);
      Serial.print(" - length:  ");
      Serial.print(_bytes,DEC);
      Serial.print(" bits");
      Serial.print(" - address:  ");
      Serial.println(_address,DEC);
      irsend.sendPanasonic(code, _bytes,false);
    }else if(strcmp(_type,"Electra_AC") == 0){
      irsend.sendElectraAC(pwr_off,12,1);
    }

    

}

void loop() {
  if (irrecv.decode(&results)) {
    String irValue;
    String rawData = resultToSourceCode(&results);
    dump(&results,rawData);
    irrecv.resume();  // Receive the next value
  }
  client.loop();
  client.publish("ping","keepalive");
  delay(100);
}
