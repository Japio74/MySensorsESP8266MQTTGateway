#include"LedControl.h"
#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, 5, NEO_GRB + NEO_KHZ800);

#define TRIGGER_LED_TIMEOUT (300)
void execute_post();
void handle_triggered_leds();


typedef enum
{
  LED_APP_MODE,         // Green = application, red = access_point or transport failure
  LED_WIFI_CONNECTION,  // Green Wifi is connected (application mode), RED: wifi is not connected
  LED_WIFI_STRENGTH,    // Green: Optimal strength, Yellow, orange, red = bad strength
  LED_TRANSPORT_ERROR,  // Off: transport is ok, Red: error during nrf transport
  LED_MQTT,             // Green: MQTT connection is present, Red: MQTT is not connected, blue: mqtt data sent or received
  LED_DATA_RECEIVED,    // Flash blue: Sensor data is received,
  LED_DATA_SENT,        // Flash blue when data sent to sensornetwork,  flash red: transmit error
  LED_CONTR_SEND_REC,   // Blue when data received from controller, Green when data sent to controller, Red: 
  LED_MAX
} LED_ALLOCATION;

typedef struct 
{
  unsigned long triggerStart;
  uint32_t activeColor;
  uint32_t lastColor;
} LedStateType;


LedStateType ledState[LED_MAX];

void setup_led()
{
  strip.begin();
  strip.setBrightness(8);  
  strip.show();

  for (int i=0;i<LED_MAX;i++)
  {
    ledState[i].triggerStart = 0;
    ledState[i].activeColor=0;
    ledState[i].lastColor=0;
  }
  
    

}

void loopLed()
{
  handle_triggered_leds();
  
  strip.show();
}

void triggerMqttLed()
{
  ledState[LED_MQTT].triggerStart = millis();
  ledState[LED_MQTT].activeColor = strip.Color(0, 0, 255);
  ledState[LED_MQTT].lastColor = strip.getPixelColor(LED_MQTT);
}
void triggerSentLed()
{
  ledState[LED_DATA_SENT].triggerStart = millis();
  ledState[LED_DATA_SENT].activeColor = strip.Color(0, 0, 255);  
}
void triggerTxError()
{
  ledState[LED_DATA_SENT].triggerStart = millis();
  ledState[LED_DATA_SENT].activeColor = strip.Color(255, 0, 0);
}

void triggerReceiveLed()
{
  
  ledState[LED_DATA_RECEIVED].triggerStart = millis();
  ledState[LED_DATA_RECEIVED].activeColor = strip.Color(0, 0, 255);
}

void set_gateway_mode_application(bool app_mode)
{
  if (app_mode == true)
  {
    strip.setPixelColor(LED_APP_MODE, 0, 255, 0);
  }
  else
  {
    strip.setPixelColor(LED_APP_MODE, 255, 0, 0);
  }
  strip.show();
  
}
void setTransportInitError(bool error)
{
  if (error == true)
  {
    strip.setPixelColor(LED_TRANSPORT_ERROR, 255, 0, 0);
  }
  else
  {
    strip.setPixelColor(LED_APP_MODE, 0, 0, 0);
  }
  
}

void handle_triggered_leds()
{
  for (int i=0;i<LED_MAX;i++)
  {
    if (ledState[i].triggerStart != 0)
    {
      if (ledState[i].triggerStart + TRIGGER_LED_TIMEOUT < millis())
      {
        ledState[i].triggerStart = 0;
        ledState[i].activeColor = strip.Color(0,0,0);
        strip.setPixelColor(i, ledState[i].lastColor);
      }
      else
      {
        strip.setPixelColor(i, ledState[i].activeColor);
      }
    }
  }  
}

void setWifiLed(bool connected)
{
  if (connected)
  {
    strip.setPixelColor(LED_WIFI_CONNECTION,0, 255, 0);
  }
  else
  {
    strip.setPixelColor(LED_WIFI_CONNECTION,255, 0, 0);
  }
  
}
void setWifiStrengthLed(int strength)
{
 
  if (strength > -67)
  {
    strip.setPixelColor(LED_WIFI_STRENGTH,0, 255, 0);   // Green
  }
  else if (strength > -70)
  {
        strip.setPixelColor(LED_WIFI_STRENGTH,127, 127, 0); // Yellow
  }
  else if (strength > -80 )
  {
        strip.setPixelColor(LED_WIFI_STRENGTH,255, 192, 255); // Orange
  }
  else
  {
    strip.setPixelColor(LED_WIFI_STRENGTH,255, 0, 0);       
  }
  
}

void setControllerConnectedLed(bool connected)
{
  if (connected)
  {
    strip.setPixelColor(LED_MQTT,0, 255, 0);
  }
  else
  {
    strip.setPixelColor(LED_MQTT,255, 0, 0);
  }
  
}


