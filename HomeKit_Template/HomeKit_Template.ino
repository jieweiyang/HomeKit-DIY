
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include "Common.h"


/* User define section
    for user to change basic settings
*/
//MQTT Server
IPAddress server(192, 168, 1, 50);
//Name of Mixture
String strClientName = "ESP8266";

/*
   Most of the time, no need to touch below settings.
*/

//Var for interval timmer
#define INTERVAL_IDLE 500
#define INTERVAL_PUBLISH 5000

//MQTT Topics
String strTopicToSet = "homebridge/to/set";
String strTopicToAdd = "homebridge/to/add";
String strTopicToGet = "homebridge/to/get";
String strTopicFrSet = "homebridge/from/set";

//Homekit Service
String strSwitch = "LightBulb";

// Other useful global var.
unsigned long timer_idle, timer_pub;
//unsigned long time_interval_pub, time_interval;
unsigned long buttonHold = 0;
int buttonState = 0;
int buttonState_pre = 0;
String strClientMAC;


// ========== General Initial settings

LED LED_status(2);        // GPIO 2, LED pin, invert with ESP8266 onboard LED.
BUTTON Button_press(3);   // GPIO 3, using RX pin, required  SERIAL_TX_ONLY Only.

//#define PIXEL_PIN 0     // GPIO 0, FastLED IO.


// Initialize the Ethernet client object
WiFiClient espClient;
PubSubClient mqttClient(espClient);


// =========== Main Program===========

void setup() {
  // initialize serial for debugging
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);

  WiFi.begin();
  WiFi.printDiag(Serial);
  Serial.setDebugOutput(true);

  LED_status.on();

  if (Button_press.status() == LOW ) {
    WiFi.disconnect();
    WiFi.beginSmartConfig();
    Serial.println("Call SmartConfig");
  }

  byte mac[6];
  WiFi.macAddress(mac);
  char str[19];
  sprintf(str, "%02x%02x%02x%02x%02x%02x", mac[0],
          mac[1], mac[2], mac[3], mac[4], mac[5]);
  strClientMAC = (String(str));

  mqttClient.setServer(server, 1883);
  mqttClient.setCallback(callback);

  Serial.println();

  /*  Output for Sensor Device Name
    Serial.println(strSensorTemp + "_" + strClientMAC);
    Serial.println(strSensorHumi + "_" + strClientMAC);
  */
  Serial.println(strSwitch + "_" + strClientMAC);

}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFiConnect();
  }

  // Interval for KEEPALIVE and READY
  if ((millis() - timer_idle) > INTERVAL_IDLE) {
    timer_idle = millis();
    if (!mqttClient.connected()) {
      Serial.println("Disconnected");
      mqttConnect();
    }
  }

  // Interval for publish
  if ((millis() - timer_pub) > INTERVAL_PUBLISH) {
    timer_pub = millis();
  }

  buttonState = Button_press.status();
  if (buttonState == LOW && buttonState_pre == 0) {
    Serial.println("ButtonPrss");
    delay(30);
    buttonState_pre = 1;
    buttonHold = millis();
    LED_status.off();

  }
  /*   Button Long press
    else if (buttonState == LOW && buttonState_pre == 1) {
    if ((millis() - buttonHold) > 3000) {                 // Button Hold for 3 second,
      Serial.println("SmartStart");                     // Disconnect wifi and wait for ESPTouch
      WiFi.disconnect();
      delay(500);
      WiFi.beginSmartConfig();
      delay(500);
      //status = WiFi.status();
    }
    }
  */
  if (buttonState == HIGH && buttonState_pre == 1) {  // Button Release
    LED_status.on();
    buttonState_pre = 0;
    buttonHold = 0;
  }

  mqttClient.loop();
}

void mqttConnect() {
  // Loop until we're reconnected
  Serial.println("Attempting MQTT connection...");
  String ClientName = strClientName + "_" + strClientMAC;
  while (!mqttClient.connected()) {
    Serial.print(".");
    // Attempt to connect, just a name to identify the client
    if (mqttClient.connect(ClientName.c_str())) {
      Serial.println();
      Serial.println("connected");

      // Once connected, publish an announcement...
      //mqttClient.publish("homebridge/from/connected", strClientName.c_str());

      /* Once connected, add Sensor
          void AddService(PubSubClient mqttClient, String strTopic, String strName, String strService);
          void AddService(PubSubClient mqttClient, String strTopic, String strName, String strService, String strCharacteristcs);
      */

      //AddService(mqttClient, strTopicToAdd, strSwitch + "_" + strClientMAC, "Lightbulb" , "Brightness,Hue,Saturation" );

      // Subscribe to channel
      mqttClient.subscribe(strTopicFrSet.c_str());

      //Serial.println("Subscribed");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
  Serial.println();
}


void callback(char* topic, byte * payload, unsigned int length) {
  LED_status.off();

  String strInPayLoad, strInTopic;
  int Tempb;
  //Serial.print("[");
  Serial.print(topic);
  strInTopic = String(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    strInPayLoad += String((char)payload[i]);
  }
  Serial.println();

  /*
    Code below are for received operation;
  */

  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(strInPayLoad);

  if (strInTopic.indexOf(String("from/set")) > 0 ) {
    String strName = root["name"];


  } // End Topic FromSet

  // Topic from Respoinse
  if (strInTopic.indexOf(String("from/response")) > 0 ) {

  } // End Topic Response

  LED_status.on();
}

//========== Functions


void WiFiConnect() {
  Serial.println();
  Serial.println("Waitting to connect...");
  while ( WiFi.status() != WL_CONNECTED)  {
    LED_status.blink(3);
    Serial.print(".");
    delay(1000);

  }
  Serial.println();
  Serial.println("You're connected to the network");
  LED_status.on();
}

