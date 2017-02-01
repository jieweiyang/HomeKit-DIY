#ifndef Common_h
#define Common_h
#include <PubSubClient.h>
#include "Arduino.h"

void AddService(PubSubClient mqttClient, String strTopic, String strName, String strService);
void AddService(PubSubClient mqttClient, String strTopic, String strName, String strService, String strCharacteristcs);



class LED
{
  public:
    LED(int pin);
    int status();
    void blink(int reps);
    void blink(int reps, int interval);
    void invert();
    void on();
    void off();
  private:
    int _pin;
};

class SWITCH
{
  public:
    SWITCH(int pin);
    boolean status();
    void invert();
    void on();
    void off();
  private:
    int _pin;
};

class BUTTON
{
  public:
    BUTTON(int pin);
    int status();
  private:
    int _pin;
};
#endif

