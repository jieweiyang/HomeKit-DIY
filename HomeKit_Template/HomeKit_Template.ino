

//Mandatory Library
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include "Common.h"
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <Ticker.h>

//Device specified Library



/* User define section
    for user to change basic settings
*/
//MQTT Server
IPAddress server(192, 168, 1, 50);
//Name of Mixture
String strClientName = "ESP8266";


#define pinLED 13
#define pinButton 0
#define pinGPIO 12

/*
   Most of the time, no need to touch below settings.
*/

//Var for interval timmer
#define INTERVAL_KEEPALIVE 5000
#define INTERVAL_PUBLISH 5000

//MQTT Topics
String strTopicToSet = "homebridge/to/set";
String strTopicToAdd = "homebridge/to/add";
String strTopicToGet = "homebridge/to/get";
String strTopicFrSet = "homebridge/from/set";

//Homekit Service
String strSwitch = "Switch";

// Other useful global var.
unsigned long time_interval, time_check;
unsigned long time_interval_pub, time_check_pub;
unsigned long buttonHold = 0;
int buttonState = 0;
int buttonState_pre = 0;
String strClientMAC;
String strDeviceName;
String strMQTTServer;
char chaTemp[20];
int PressTicCount = 0;
int PressTicSwitch = 0;
int PressTicStatus = 0;
int PressHold = 0;

float STATUS_NOWIFI = 2;
float STATUS_HOLD4 = 0.2;
float STATUS_PORTAL = 0.5;




// ========== General Initial settings

LED LED_status(pinLED);        // LED pin, invert with ESP8266 onboard LED.
BUTTON Button_press(pinButton);   //  using RX pin, required  SERIAL_TX_ONLY Only.

SWITCH relay(pinGPIO);

// Initialize the Ethernet client object
WiFiClient espClient;
PubSubClient mqttClient(espClient);
WiFiManager wifiManager;
WiFiManagerParameter mqtt_server("server", "MQTT Server", "192.168.1.50", 40);

Ticker ticLED;
Ticker ticPressCount;
Ticker ticPressHold;


// =========== Main Program===========

void setup() {
  // initialize serial for debugging
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);

  WiFi.begin();
  EEPROM.begin(20);

  LED_status.on();
  relay.off();
  ticLED.attach(STATUS_NOWIFI, ticCallLED);

  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setTimeout(120);
  wifiManager.addParameter(&mqtt_server);

  byte mac[6];
  WiFi.macAddress(mac);
  char str[19];
  sprintf(str, "%02x%02x%02x%02x%02x%02x", mac[0],
          mac[1], mac[2], mac[3], mac[4], mac[5]);
  strClientMAC = (String(str));

  int intEEPLength = EEPROM.read(0);
  String strMQTTServer;
  if (intEEPLength != 0) {
    for (int i = 1; i <= intEEPLength; i++) {
      strMQTTServer += char(EEPROM.read(i));
    }
  }
  Serial.print("Readingfrom EEPROM:");
  Serial.println(strMQTTServer);
  strMQTTServer.toCharArray(chaTemp,15);

  mqttClient.setServer(chaTemp, 1883);
  mqttClient.setCallback(callback);


  time_interval_pub = INTERVAL_PUBLISH;
  time_interval = INTERVAL_KEEPALIVE;
  strDeviceName = strSwitch + "_" + strClientMAC;

  Serial.println(strDeviceName);
  Serial.println("Setup Completed");


}

void loop() {

  /*if (PressTicSwitch != 0) {
    //Serial.print("ButtonPressCount:");
    //Serial.println(PressTicSwitch);
    ticPressCount.detach();
    //Serial.println("PressCount Detached");

    switch (PressTicSwitch) {
      case 6:
        if (!wifiManager.startConfigPortal(strDeviceName.c_str())) {
          //Serial.println("failed to connect and hit timeout");
        }
        ticLED.attach(0.6, ticCallLED);
        break;
      case 8:
        ESP.reset();
        break;
    }

    // Reset Counters
    PressTicSwitch = 0;
    PressTicCount = 0;


    }*/

  buttonState = Button_press.status();
  if (buttonState == LOW && buttonState_pre == 0) {
    LED_status.off();

    pub_SwitchStatus(strTopicToSet, strSwitch + "_" + strClientMAC, relay);
    buttonState_pre = 1;


    /* if (PressTicStatus == 0) {
       PressTicStatus = 1;
       ticPressCount.attach(1.5, ticCallPressCount);
       //Serial.println("PressCount Attached");
      }
      PressTicCount++;*/

    ticPressHold.attach(1, ticCallPressHold);

  }


  if (buttonState == HIGH && buttonState_pre == 1) {  // Button Release
    // Serial.println("ButtonRelease");
    //Serial.println("ButtonPrss");
    if (PressHold < 1) {  // ShorPress
      relay.invert();
    }

    LED_status.on();
    ticPressHold.detach();
    //Serial.print("Button Hold for:");
    //Serial.println(PressHold);

    if (PressHold >= 4 && PressHold < 8) {
      if (!wifiManager.startConfigPortal(strDeviceName.c_str())) {
        Serial.println("failed to connect and hit timeout");
        ticLED.attach(STATUS_NOWIFI, ticCallLED);
      }
      else {
        Serial.print("Get MQTT Addr:");
        Serial.println(mqtt_server.getValue());

        strMQTTServer = String(mqtt_server.getValue());

        EEPROM.write(0, strMQTTServer.length());
        for (int i = 1; i <= strMQTTServer.length(); i++) {
          EEPROM.write(i, strMQTTServer.charAt(i - 1));
        }
        EEPROM.commit();
        Serial.println("MQTT Server saved");
        ESP.reset();

      }
    }
    if (PressHold >= 8) {
      ESP.reset();
    }

    buttonState_pre = 0;
    buttonHold = 0;
    PressHold = 0;
  }



  if (WiFi.status() == WL_CONNECTED) {   // If Wifi connected, proceed with

    if (buttonState_pre == 0) { // If button not holding. Stop LED blinking.
      ticLED.detach();
      LED_status.on();
    }

    if ((millis() - time_check) > time_interval) {
      time_check = millis();
      if (!mqttClient.connected()) {
        Serial.println("Disconnected");
        mqttConnect();
      }
    }

    // Interval for publish
    if ((millis() - time_check_pub) > time_interval_pub) {
      time_check_pub = millis();
      LED_status.off();

      LED_status.on();
    }


    mqttClient.loop();
  }

}

/*
   General Functions

*/


//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {

  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticLED.attach(STATUS_PORTAL, ticCallLED);
}


/*
   Ticker Functions, based on interrupt.

   Blocking function prohibted.
   e.g. delay();
*/
void ticCallLED() {
  //toggle state
  int state = digitalRead(pinLED);  // get the current state of GPIO1 pin
  digitalWrite(pinLED, !state);     // set pin to the opposite state
}

void ticCallPressCount() {
  PressTicSwitch = PressTicCount;
  PressTicStatus = 0;
}

void ticCallPressHold() {
  PressHold++;
  if (PressHold >= 4) {
    ticLED.attach(STATUS_HOLD4, ticCallLED);
  }
}


/*
   MQTT functions

   mqttConnect() - MQTT server connect, client registery, accessory registery.
   callback() - Processing MQTT message from server broadcast.

*/

void mqttConnect() {
  // Loop until we're reconnected
  Serial.println("Attempting MQTT connection...");
  String ClientName = strClientName + "_" + strClientMAC;
  //while (!mqttClient.connected()) {
  // Serial.print(".");
  // Attempt to connect, just a name to identify the client
  if (mqttClient.connect(ClientName.c_str())) {
    Serial.println();
    Serial.println("connected");

    // Once connected, publish an announcement...
    //mqttClient.publish("homebridge/from/connected", strClientName.c_str());

    /* Once connected, add Accessory
        void AddService(PubSubClient mqttClient, String strTopic, String strName, String strService);
        void AddService(PubSubClient mqttClient, String strTopic, String strName, String strService, String strCharacteristcs);
    */

    //AddService(mqttClient, strTopicToAdd, strSwitch + "_" + strClientMAC, "Lightbulb" , "Brightness,Hue,Saturation" );
    AddService(mqttClient, strTopicToAdd, strDeviceName, "Switch");


    // Subscribe to channel
    mqttClient.subscribe(strTopicFrSet.c_str());

    //Serial.println("Subscribed");
  } else {
    Serial.print("failed, rc=");
    Serial.print(mqttClient.state());

  }
  //}
  Serial.println();
}


void callback(char* topic, byte * payload, unsigned int length) {


  String strInPayLoad, strInTopic;
  int Tempb;
  Serial.print("[");
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
    if (strName == strDeviceName ) {
      LED_status.off();
      String strChar = root["characteristic"];
      boolean bolVal = root["value"];

      if (strChar == String("On") && bolVal == 0) {
        relay.off();
      }
      else if (strChar == String("On") && bolVal == 1) {
        relay.on();
      }
    }

    LED_status.on();
  } // End Topic FromSet

  // Topic from Respoinse
  if (strInTopic.indexOf(String("from/response")) > 0 ) {

  } // End Topic Response


}

void pub_SwitchStatus(String strSetTopic, String strButtonName, SWITCH _switch ) {

  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["name"] = strButtonName;
  root["characteristic"] = "On";
  root["value"] = _switch.status();

  String strJSON;
  root.printTo(strJSON);

  Serial.print("[");
  Serial.print(strSetTopic);
  Serial.print("] ");
  Serial.println(strJSON);

  mqttClient.publish(strSetTopic.c_str(), strJSON.c_str());
  mqttClient.loop();
}



