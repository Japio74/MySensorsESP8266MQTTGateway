
/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * REVISION HISTORY
 * Version 1.0 - Henrik Ekblad
 * 
 * DESCRIPTION
 * The ESP8266 MQTT gateway sends radio network (or locally attached sensors) data to your MQTT broker.
 * The node also listens to MY_MQTT_TOPIC_PREFIX and sends out those messages to the radio network
 *
 * LED purposes:
 * - To use the feature, uncomment WITH_LEDS_BLINKING in MyConfig.h
 * - RX (green) - blink fast on radio message recieved. In inclusion mode will blink fast only on presentation recieved
 * - TX (yellow) - blink fast on radio message transmitted. In inclusion mode will blink slowly
 * - ERR (red) - fast blink on error during transmission error or recieve crc error  
 * 
 * See http://www.mysensors.org/build/esp8266_gateway for wiring instructions.
 * nRF24L01+  ESP8266
 * VCC        VCC
 * CE         GPIO4          
 * CSN/CS     GPIO15
 * SCK        GPIO14
 * MISO       GPIO12
 * MOSI       GPIO13
 *            
 * Not all ESP8266 modules have all pins available on their external interface.
 * This code has been tested on an ESP-12 module.
 * The ESP8266 requires a certain pin configuration to download code, and another one to run code:
 * - Connect REST (reset) via 10K pullup resistor to VCC, and via switch to GND ('reset switch')
 * - Connect GPIO15 via 10K pulldown resistor to GND
 * - Connect CH_PD via 10K resistor to VCC
 * - Connect GPIO2 via 10K resistor to VCC
 * - Connect GPIO0 via 10K resistor to VCC, and via switch to GND ('bootload switch')
 * 
  * Inclusion mode button:
 * - Connect GPIO5 via switch to GND ('inclusion switch')
 * 
 * Hardware SHA204 signing is currently not supported!
 *
 * Make sure to fill in your ssid and WiFi password below for ssid & pass.
 */

#include <EEPROM.h>
#include <SPI.h>
#include <IPAddress.h>

// Enable debug prints to serial monitor
#define MY_DEBUG 

// Use a bit lower baudrate for serial prints on ESP8266 than default in MyConfig.h
#define MY_BAUD_RATE 9600

// Enables and select radio type (if attached)
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#define MY_GATEWAY_MQTT_CLIENT
#define MY_GATEWAY_ESP8266

// Set this nodes subscripe and publish topic prefix
#define MY_MQTT_PUBLISH_TOPIC_PREFIX "mys-mqtt-out"
#define MY_MQTT_SUBSCRIBE_TOPIC_PREFIX "mys-mqtt-in"

// Set MQTT client id
#define MY_MQTT_CLIENT_ID "mysensors-gateway"

// Enable these if your MQTT broker requires usenrame/password
//#define MY_MQTT_USER "username"
//#define MY_MQTT_PASSWORD "password"

// Set WIFI SSID and password
char MY_ESP8266_SSID[33];
char MY_ESP8266_PASSWORD[64];

// Set the hostname for the WiFi Client. This is the hostname
// it will pass to the DHCP server if not static. 
#define MY_ESP8266_HOSTNAME "MySensors-Mqtt-gateway"

// Enable MY_IP_ADDRESS here if you want a static ip address (no DHCP)
//#define MY_IP_ADDRESS 192,168,178,87

// If using static ip you need to define Gateway and Subnet address as well
//#define MY_IP_GATEWAY_ADDRESS 192,168,178,1
//#define MY_IP_SUBNET_ADDRESS 255,255,255,0


// MQTT broker ip address.  
#define MY_CONTROLLER_IP_ADDRESS 0,0,0,0
 
// Flash leds on rx/tx/err
//#define MY_LEDS_BLINKING_FEATURE
// Set blinking period
//#define MY_DEFAULT_LED_BLINK_PERIOD 300

// Enable inclusion mode
//#define MY_INCLUSION_MODE_FEATURE
// Enable Inclusion mode button on gateway
//#define MY_INCLUSION_BUTTON_FEATURE
// Set inclusion mode duration (in seconds)
//#define MY_INCLUSION_MODE_DURATION 60 
// Digital pin used for inclusion mode button
//#define MY_INCLUSION_MODE_BUTTON_PIN  3 

#include <ESP8266WiFi.h>
#include <MySensors.h>
#include "GlobalTypes.h"
#include "NtpClient.h"
#include "WebServer.h"
#include "GatewayESP8266MQTTClient.h"
#include "LedControl.h"
#include "PersistentData.h"
#include "Gpio.h"

unsigned long gwMsgSent = 0;
unsigned long gwMsgRec = 0;
unsigned long sensMsgSent = 0;
unsigned long sensMsgRec = 0;
int resettimeout = 0;

extern IPAddress _brokerIp;
#undef MY_PORT
#undef MY_ESP8266_SSID
#undef MY_ESP8266_PASSWORD

int MY_PORT = 0;


nodeInfoType nodeInfo[MAX_NODES];
sensorInfoType sensorInfo[MAX_SENSORS];
bool WifiLedState = false;

void handlePresentationMessage(const MyMessage& message);
void handleInternalMessage(const MyMessage& message);
void handleSetMessage(const MyMessage& message);
int findNodeSlot(int NodeId);
int findSensorSlot(int NodeId, int SensorId);
void clearSensorInfo(int Node);
void InitializeDefaultBrokerIPAddress();
void APModeSetup();
void APModeLoop();

void before()
{
  for (int i=0;i < MAX_NODES;i++)
  {
    nodeInfo[i].sketchName = "";
    nodeInfo[i].nodeId = -1;
    nodeInfo[i].presentationTime = 0;
  }

  for (int i=0;i < MAX_SENSORS;i++)
  {
    sensorInfo[i].nodeId = -1;
    sensorInfo[i].sensorId = -1;
    sensorInfo[i].type = -1;
    sensorInfo[i].presented = 0;
    sensorInfo[i].lastSet = 0;
  }

  setup_led();

  if (!IsInitialized())
  {
    Serial.println("Going to Access Point mode");
    set_gateway_mode_application(false);
    APModeSetup();

    while(1)
    {
      APModeLoop();
      delay(1); // Trigger Watchdog
    }
  }
  else
  {
    uint8_t brokerIp[4];
    int brokerPort;
    String ssid;
    String password;
    
    Serial.println("Going to MySensors Application mode");
    set_gateway_mode_application(true);
    setWifiLed(false);
    WifiLedState = false;
    setControllerConnectedLed(false);
    loopLed();          //  execute strip.show();
    
    ReadBrokerIPAddress(brokerIp);
    Serial.print("- Broker Ip Address: "); 
    for (int i=0;i<4;i++)
    { 
      Serial.print(brokerIp[i]);
      if (i<3) Serial.print("."); else Serial.println("");
    }
    _brokerIp = brokerIp;

    brokerPort = ReadBrokerPort();
    Serial.print("- Broker Port: "); Serial.println(brokerPort);
    MY_PORT = brokerPort;

    ReadNetworkName(ssid);
    ReadNetworkPass(password);
    Serial.print("- Network name: ");     Serial.println(ssid); 

    ssid.toCharArray(MY_ESP8266_SSID, 32);
    password.toCharArray(MY_ESP8266_PASSWORD, 64);
      
  }
}
void setup() 
{
  
  Serial.begin(9600);

  setup_WebServer();
  setup_ntp();
  Gpio_setup();
  setWifiLed(WifiLedState);
}


void indication( const indication_t ind )
{
  switch (ind)
  {
    case INDICATION_GW_TX:
      triggerMqttLed();
      gwMsgSent++;
      break;

    case INDICATION_GW_RX:
      triggerMqttLed();
      gwMsgRec++;
      break;

    case INDICATION_ERR_TX:
      triggerTxError();
      break;  
    case INDICATION_ERR_INIT_TRANSPORT:
      setTransportInitError(true);
      break;

    case INDICATION_TX:
      sensMsgSent++;
      triggerSentLed();
      break;
      
    case INDICATION_RX:
      sensMsgRec++;
      triggerReceiveLed();
      break;
      
    default:
    break;
  };
}

void presentation() {
  // Present locally attached sensors here    
}

void loop() {
  // Send locally attech sensors data here

  loop_WebServer();
  loopNtp();
  Gpio_loop();
  
  setControllerConnectedLed(_MQTT_client.connected() == true);
  loopLed();

  if (!_MQTT_client.connected())
  {
    if (resettimeout == 0)
    {
      resettimeout = millis();
    }
    else if ((resettimeout + 10000) > millis())
    {
      ESP.restart();
    }
  }
  else
  {
    resettimeout = 0;
  }
}

void sendResetRequest(int node)
{
  Serial.print("reset request for node: ");
  Serial.println(node);

  MyMessage msg(254, V_STATUS );
  
  msg.setDestination(node);
  msg.set((uint8_t)1);
  send(msg, false);
  
}
void receive(const MyMessage &message)
{

  
  switch (mGetCommand(message))
  {
    case C_PRESENTATION:
      Serial.println("PresentationMessage");
      handlePresentationMessage(message);
      break;

  case C_INTERNAL:
    handleInternalMessage(message);
    break;
      default:
      break;

   case C_SET:
    handleSetMessage(message);
    break;
  }
 
}

void handlePresentationMessage(const MyMessage& message)
{
  int slot=findSensorSlot(message.sender, message.sensor);
  sensorInfo[slot].type = message.type;
  sensorInfo[slot].nodeId = message.sender;
  sensorInfo[slot].sensorId = message.sensor;
  sensorInfo[slot].presented = getTime();

  if (message.sensor == 255)
  {
    slot=findNodeSlot(message.sender);
    nodeInfo[slot].nodeId = message.sender;
    nodeInfo[slot].mySensorsVersion = message.getString();
  }    
}
void handleInternalMessage(const MyMessage& message)
{
  int i=0;
  switch (message.type)
  {
    case I_SKETCH_NAME:
    i=findNodeSlot(message.sender);
    if (i != -1)
    {
      nodeInfo[i].sketchName = message.getString();
      nodeInfo[i].nodeId = message.sender;
      nodeInfo[i].presentationTime = getTime();

      clearSensorInfo(nodeInfo[i].nodeId);
    }

    break;

    case I_SKETCH_VERSION:
    i=findNodeSlot(message.sender);
    if (i != -1)
    {
      nodeInfo[i].sketchVersion = message.getString();
    }
    break;
    
    default:
    break;
  };
}
void clearSensorInfo(int Node)
{
  for (int i=0;i<MAX_NODES;i++)
  {
    if (sensorInfo[i].nodeId == Node)
    {
      sensorInfo[i].nodeId = -1;
      sensorInfo[i].sensorId = -1;
      sensorInfo[i].type = -1;
      sensorInfo[i].presented = 0;
      sensorInfo[i].lastSet = 0;      
    }
  }

}
void handleSetMessage(const MyMessage& message)
{
  int slot=findSensorSlot(message.sender, message.sensor);
  sensorInfo[slot].nodeId = message.sender;
  sensorInfo[slot].sensorId = message.sensor;
  sensorInfo[slot].unit = message.type;
  message.getString(sensorInfo[slot].data);
  
  sensorInfo[slot].lastSet = getTime();
}

int findNodeSlot(int NodeId)
{
  int slot = -1;
  bool bDone = false;
  
  for (int i=0;(i<MAX_NODES) && (bDone == false);i++)
  {
    if (nodeInfo[i].nodeId == NodeId)
    {
      slot = i;
      bDone = true;
    }
    if ((nodeInfo[i].nodeId == -1) && (slot == -1))
    {
      slot = i;
    }
    
  }
  return slot;
}

int findSensorSlot(int NodeId, int SensorId)
{
  int slot = -1;
  bool bDone = false;
  
  for (int i=0;(i<MAX_SENSORS) && (bDone == false);i++)
  {
    if ((sensorInfo[i].nodeId == NodeId) && (sensorInfo[i].sensorId == SensorId))
    {
      slot = i;
      bDone = true;
    }
    if ((sensorInfo[i].nodeId == -1) && (slot == -1))
    {
      slot = i;
    }
    
  }
  return slot;
}

uint8_t MyLoadState(uint8_t pos)
{
  return loadState(pos);
}

void MySaveState( uint8_t pos, uint8_t value)
{
  return saveState(pos, value);
}

void APModeSetup()
{
  Serial.println("ApModeSetup()");

  StartAP();

  setup_WebServer();

}
void APModeLoop()
{
    loop_WebServer();
 
}

