
#include <Arduino.h>
#include "Gpio.h"
#include "PersistentData.h"

bool lastGpio0State = false;
unsigned long Gpio0ActivateTime = 0;


#define TIMEOUT_BEFORE_ERASING_FLASH (10000)
void Gpio_setup()
{
  pinMode(0, INPUT);

  lastGpio0State = digitalRead(0);
  
}
void Gpio_loop()
{
  bool newGPio0State = digitalRead(0);

  if (newGPio0State!= lastGpio0State)
  {
    Serial.print("GPIO0: ");Serial.println(newGPio0State);
  }
  
  if (lastGpio0State == false)     // Button was pressed before
  {
    if (newGPio0State == true)   // But now it is released
    {
      Serial.println("Button is released");
      if((millis() - Gpio0ActivateTime) > TIMEOUT_BEFORE_ERASING_FLASH)
      {
        Serial.println("Invalidate configuration flash, restart gateway to activate access point");
        InvalidateData();
        Gpio0ActivateTime = 0;  
        
      }
    }
  }
  else    // last state was not pressed
  {
    if (newGPio0State == false)
    {
      Serial.println("Erase flash button is pressed");
      Gpio0ActivateTime = millis();
    }
  }
  lastGpio0State = newGPio0State;

}

