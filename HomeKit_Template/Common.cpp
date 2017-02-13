#include "Arduino.h"
#include "Common.h"
#include <PubSubClient.h>
#include <ArduinoJson.h>

void AddService(PubSubClient mqttClient, String strTopic, String strName, String strService) {

  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["name"] = strName;
  root["service"] = strService;

  String strJSON;
  root.printTo(strJSON);
  Serial.println(strJSON);
  mqttClient.publish(strTopic.c_str(), strJSON.c_str());
  mqttClient.loop();
}

void AddService(PubSubClient mqttClient, String strTopic, String strName, String strService, String strCharacteristcs) {

  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["name"] = strName;
  root["service"] = strService;


  int lastindex = 0, posStart = 0, posEnd = 0;
  String strAddChar;

  do {
    lastindex = posEnd;
    posStart = posEnd ;
    posEnd = strCharacteristcs.indexOf(",", lastindex + 1);
    if (posStart == 0) {
      strAddChar = strCharacteristcs.substring(posStart, posEnd);

    }
    else {
      strAddChar = strCharacteristcs.substring(posStart + 1, posEnd);
    }
    root[strAddChar] = "default";
    //Serial.println(posEnd);
  } while (posEnd > 0);


  String strJSON;
  root.printTo(strJSON);
  Serial.println(strJSON.c_str());
  //mqttClient.publish(strTopic.c_str(), strJSON.c_str(), strJSON.length());
  mqttClient.publish(strTopic.c_str(), strJSON.c_str());
  mqttClient.loop();
}





LED::LED(int pin) {
  pinMode(pin, OUTPUT);
  _pin = pin;
}

int LED::status() {
  return digitalRead(_pin);
}

void LED::blink(int reps) {
  digitalWrite(_pin, LOW);
  for (int i = 1; i <= reps; i++) {
    digitalWrite(_pin, HIGH);
    delay(150);
    digitalWrite(_pin, LOW);
    delay(150);
  }
  digitalWrite(_pin, HIGH);
}

void LED::blink(int reps, int interval) {
  digitalWrite(_pin, LOW);
  for (int i = 1; i <= reps; i++) {
    digitalWrite(_pin, HIGH);
    delay(interval);
    digitalWrite(_pin, LOW);
    delay(interval);
  }
  digitalWrite(_pin, HIGH);
}

void LED::invert() {
  int bolStatusCurrent = digitalRead(_pin);
  if (bolStatusCurrent == HIGH) {
    digitalWrite(_pin, LOW);
  }
  else {
    digitalWrite(_pin, HIGH);
  }
}

void LED::on() {
  digitalWrite(_pin, HIGH);
}

void LED::off() {
  digitalWrite(_pin, LOW);
}

//  =======================Switch

SWITCH::SWITCH(int pin) {
  pinMode(pin, OUTPUT);
  _pin = pin;
}

boolean SWITCH::status() {
  return digitalRead(_pin);
}


void SWITCH::invert() {
  int bolStatusCurrent = digitalRead(_pin);
    digitalWrite(_pin, !bolStatusCurrent);

}

void SWITCH::on() {
  digitalWrite(_pin, HIGH);
}

void SWITCH::off() {
  digitalWrite(_pin, LOW);
}


// ============ Button

BUTTON::BUTTON(int pin) {
  pinMode(pin, INPUT);
  _pin = pin;
}

int BUTTON::status() {
  return digitalRead(_pin);
}

