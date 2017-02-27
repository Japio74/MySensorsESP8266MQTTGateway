
#include <stdint.h>
#include <Arduino.h>
#include "GatewayESP8266MQTTClient.h"
#include "PersistentData.h"

#define MAGIC_BYTE (0xCC)

#define MAGIC_BYTE_OFFSET (0)
#define MAGIC_BYTE_SIZE (1)

#define BROKER_IPADDRESS_OFFSET (MAGIC_BYTE_OFFSET + MAGIC_BYTE_SIZE)
#define BROKER_IPADDRESS_SIZE (4)
#define BROKER_PORT_OFFSET (BROKER_IPADDRESS_OFFSET + BROKER_IPADDRESS_SIZE)
#define BROKER_PORT_SIZE (2)
#define NW_OFFSET (BROKER_PORT_OFFSET + BROKER_PORT_SIZE)
#define NW_SIZE (33)
#define NW_PASS_OFFSET (NW_OFFSET + NW_SIZE)
#define NW_PASS_SIZE (64)

bool IsInitialized()
{
    uint8_t Ebyte = MyLoadState(MAGIC_BYTE_OFFSET);
    bool bInitOk = true;

    //check magic byte first
    if (Ebyte != MAGIC_BYTE)
    {
      bInitOk = false;
      Serial.println("Persistent data check failed on magic byte");
    }

    //check for valid network name
    if (bInitOk == true)
    {
      uint8_t brokerIp[4];
      ReadBrokerIPAddress(brokerIp);
      for (int i=0;i<4;i++)
      {
        if (!((brokerIp[i] > 0) && (brokerIp[i] < 255)))
        {
          bInitOk = false;
          Serial.println("Persistent data check failed on broker IP address");
        }
      }
    }
    // Check netowrk name
    if (bInitOk == true)
    {
      if (MyLoadState(NW_OFFSET) == 0xFF)
      {
          bInitOk = false;
          Serial.println("Persistent data check failed on network name");
      }
    }      
    if (bInitOk == true)
    {
      if (MyLoadState(NW_PASS_OFFSET) == 0xFF)
      {
          bInitOk = false;
          Serial.println("Persistent data check failed on network password");
      }
    }

    return bInitOk;
}

void ReadNetworkName(String& networkName)
{
  networkName = "";
  char token[2];
  int idx=0;

  do
  {
    token[0] = MyLoadState(NW_OFFSET + idx++);
    token[1] = '\0';
    if (token[0] != '\0')
    {
      networkName+=token;
    }
  } while ((token[0] != '\0') && (idx < NW_SIZE)) ;
}
void ReadNetworkPass(String& networkPass)
{
  networkPass = "";
  char token[2];
  int idx=0;

  do
  {
    token[0] = MyLoadState(NW_PASS_OFFSET + idx++);
    token[1] = '\0';
    if (token[0] != '\0')
    {
      networkPass+=token;
    }
  } while ((token[0] != '\0') && (idx < NW_PASS_SIZE)) ;
  
}

void ReadBrokerIPAddress(uint8_t* pIPAddress)
{

  for (int i=0;i<4;i++)
  {
    pIPAddress[i] = MyLoadState(BROKER_IPADDRESS_OFFSET+i);
  }

}

int ReadBrokerPort()
{
  return (MyLoadState(BROKER_PORT_OFFSET) << 8) + MyLoadState(BROKER_PORT_OFFSET+1);
}

void InitializeDefaultBrokerIPAddress(uint8_t* pIPAddress)
{
  for (int i=0;i<4;i++)
  {
    MySaveState(BROKER_IPADDRESS_OFFSET+i, pIPAddress[i]);
  }
}
void WriteMagicByte()
{
  MySaveState(MAGIC_BYTE_OFFSET, MAGIC_BYTE);
}

void WriteBrokerIP(uint8_t *pIp)
{
  for (int i=0;i<4;i++)
  {
    MySaveState(BROKER_IPADDRESS_OFFSET+i, pIp[i]);
  }
}

void WriteBrokerPort(int16_t port)
{
  MySaveState(BROKER_PORT_OFFSET, port >> 8);
  MySaveState(BROKER_PORT_OFFSET + 1, port & 0xFF);
  
}

void WriteNetworkName(String NetworkName)
{
  for (int i=0;i < NW_SIZE;i++)
  {
    if ((i < NetworkName.length()) && (i < NW_SIZE))
    {
      MySaveState(NW_OFFSET + i, NetworkName[i]);
    }
    else
    {
      MySaveState(NW_OFFSET + i, 0x00);
    }
  }
  
}
void WriteNetworkPass(String NetworkPass)
{
  for (int i=0;i < NW_PASS_SIZE;i++)
  {
    if ((i < NetworkPass.length()) && (i < NW_PASS_SIZE-1))
    {
      MySaveState(NW_PASS_OFFSET + i, NetworkPass[i]);
    }
    else
    {
      MySaveState(NW_PASS_OFFSET + i, 0x00);
    }
  }
  
}

void InvalidateData()
{
    MySaveState(MAGIC_BYTE_OFFSET, 0x00);
}

