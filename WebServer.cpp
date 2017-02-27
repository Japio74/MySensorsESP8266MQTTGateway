
#include "WebServer.h"
#include <ESP8266WebServer.h>
#include "GlobalTypes.h"
#include "GWSensorTypes.h"
#include "NtpClient.h"
#include "GatewayESP8266MQTTClient.h"
#include "PersistentData.h"
#include <WifiServer.h>
#include "LedControl.h"

ESP8266WebServer WebServer(80);
String           WebPage = "<h1>ESP8266 MQTT gateway for MySensors</h1>";
WiFiServer Server(80);


extern unsigned long gwMsgSent;
extern unsigned long gwMsgRec;
extern unsigned long sensMsgSent;
extern unsigned long sensMsgRec;

extern nodeInfoType nodeInfo[MAX_NODES];
extern sensorInfoType sensorInfo[MAX_SENSORS];

void resetButtonHandler(String node);
void showRootPage();
void showConfigPage();
void showStoredPage();
bool ParseIPAddress(String Ip, uint8_t* pBrokerIP);

void storeConfigData(String Ip, String Port, String Network, String Pass);

void StartAP()
{

   WiFi.disconnect();
   WiFi.mode(WIFI_STA);

  Serial.println("Starting accss point");
  
  WiFi.softAP("MySGateway");
}

void setup_WebServer()
{
  WebServer.on("/", []()
  {
    String node="";
    for (uint8_t i = 0; i < WebServer.args(); i++)
    {
      if (WebServer.argName(i) == "node")
      {
        node = WebServer.arg(i);
        resetButtonHandler(node);
      }
    }
    showRootPage();        
  });

  WebServer.on("/config", []()
  {
    showConfigPage();
  });

  WebServer.on("/writeConfig", []()
  {
    String Ip;
    String Port;
    String Network;
    String Pass;

    for (uint8_t i = 0; i < WebServer.args(); i++)
    {
      if (WebServer.argName(i) == "ip")
      {
        Ip = WebServer.arg(i);
      }
      if (WebServer.argName(i) == "port")
      {
        Port = WebServer.arg(i);
      }
      if (WebServer.argName(i) == "network")
      {
        Network = WebServer.arg(i);
      }
      if (WebServer.argName(i) == "pw")
      {
        Pass = WebServer.arg(i);
      }
    }
    storeConfigData(Ip, Port, Network, Pass);


  });
  
  WebServer.begin();
  Serial.println("WebServer started...");

}

void loop_WebServer()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    WiFi.begin();
    delay(500);
  }
  
  setWifiLed(WiFi.status() == WL_CONNECTED);
  setWifiStrengthLed(WiFi.RSSI());    

  WebServer.handleClient();
}

void resetButtonHandler(String node)
{
  sendResetRequest(node.toInt());
}

void showRootPage()
{
  String page = WebPage;
  page+= "<meta http-equiv=\"refresh\" content=\"20\">";
  page+="<br>Or go to the <a href=\"/config\">configuration</a> screen</br>"; 
  page+="<br><h2>General information</h2></br>";
  page+= "<style> table, th, td { border: 1px solid black;}</style>";
  page+="<table style=\"width:600\">";
  page+="<tr>"; page+=  "<th>Item</th>";        page+=  "<th>Value</th>";                                            page+="</tr>";
  page+="<tr><td>Gateway started on</td><td>"; page += getStartTime(); page+="</td></tr>";
  page+="<tr><td>It is now</td><td>";page+=getFormattedTime();page+="</td></tr>";
  page+="<tr>"; page+=  "<td>Free memory:</td>";               page+=  "<td>"; page += ESP.getFreeHeap() ; page+= " bytes</td>";                 page+="</tr>";
  page+="<tr>"; page+=  "<td>WIFI signal:</td>";               page+=  "<td>"; page += WiFi.RSSI(); ; page+= " dBm</td>";                 page+="</tr>";
  page+="<tr>"; page+=  "<td>WIFI SSID:</td>";               page+=  "<td>"; page += WiFi.SSID(); ; page+= "</td>";                 page+="</tr>";  
  page+="</table>";

  page+="<br><h2>MySensors gateway information</h2></br>";
  page+= "<style> table, th, td { border: 1px solid black;}</style>";
  page+="<table style=\"width:600\">";
  page+="<tr>"; page+=  "<th>Item</th>";        page+=  "<th>Value</th>";                                            page+="</tr>";
  page+="<tr>"; page+=  "<td>MQTT messages received</td>";     page+=  "<td>"; page += gwMsgRec; page+= "</td>";                                page+="</tr>";
  page+="<tr>"; page+=  "<td>MQTT messages sent</td>";     page+=  "<td>"; page +=     gwMsgSent; page+= "</td>";                                page+="</tr>";
  page+="<tr>"; page+=  "<td>Sensor messages received</td>";     page+=  "<td>"; page += sensMsgRec; page+= "</td>";                                page+="</tr>";
  page+="<tr>"; page+=  "<td>Sensor messages sent</td>";     page+=  "<td>"; page +=     sensMsgSent; page+= "</td>";                                page+="</tr>";
  page+="</table>";
/*
  page+="<br><h2>Node information</h2></br>";
  page+= "<style> table, th, td { border: 1px solid black;}</style>";
  page+="<table style=\"width:600\">";
  page+="<tr>"; page+=  "<th>Node Id</th><th>Sketch name</th><th>Sketch version</th><th>MySensors version</th><th>Last presented</th><th>Reset</th></tr>";

  for (int i=0;i<MAX_NODES;i++)
  {
    if (nodeInfo[i].nodeId != -1)
    {
      page+="<tr>"; page+=  "<td>" ; page+=nodeInfo[i].nodeId; page+="</td>";     
      page+=  "<td>"; page += nodeInfo[i].sketchName; page+= "</td>";
      page+=  "<td>"; page += nodeInfo[i].sketchVersion; page+= "</td>";
      page+=  "<td>"; page += nodeInfo[i].mySensorsVersion; page+= "</td>";
      page+=  "<td>"; page += formatTime(nodeInfo[i].presentationTime); page+= "</td>";
//      page+=  "<td>"; page += "<p><center><a href=\"/?node="; page+= nodeInfo[i].nodeId; page+="\"><button>Reset</button></a>&nbsp</center></p>"; page+= "</td>";
      page+="</tr>";
    }
    
  }*/
  page+="</table>";

  page+="<br><h2>Sensor information</h2></br>";
  page+= "<style> table, th, td { border: 1px solid black;}</style>";
  page+="<table style=\"width:600\">";
  page+="<tr>"; page+=  "<th>Node Id</th><th>Sensor Id</th><th>Type</th><th>Data</th><th>Data type</th><th>Last presented since</th><th>Last set message</th></tr>";

  for (int i=0;i<MAX_SENSORS;i++)
  {
    if (sensorInfo[i].nodeId != -1)
    {
      String SensprData = 
      page+="<tr>"; page+=  "<td>" ; page+=sensorInfo[i].nodeId; page+="</td>";     
      page+=  "<td>"; page += sensorInfo[i].sensorId; page+= "</td>";
      if (sensorInfo[i].type != -1)
      {
        page+=  "<td>"; page += GWsensorTypes[sensorInfo[i].type]; page+= "</td>";
      }
      else
      {
        page+=  "<td>No presentation received</td>";
      }
      
      page+=  "<td>"; page += sensorInfo[i].data; page+= "</td>";
      page+=  "<td>"; page += GWSensorUnits[sensorInfo[i].unit]; page+= "</td>";
      if (sensorInfo[i].presented > 1000)
      {
        page+=  "<td>"; page += formatTime(sensorInfo[i].presented); page+= "</td>";
      }
      else
      {
        page+=  "<td> - </td>";
      }
      if (sensorInfo[i].lastSet > 1000)
      {
        page+=  "<td>"; page += formatTime(sensorInfo[i].lastSet); page+= "</td>";page+="</tr>";
      }
      else
      {
        page+=  "<td> - </td></tr>";
      }
    }
    
  }
  page+="</table>";
//  page+= "<br><form action=\"/\" method=\"get\">";
//  page += "Reset node: <input type=\"text\" name=\"node\" size=\"3\" ><input type=\"submit\" value=\"Reset\">";
//  page += "</form>";
  WebServer.send(200, "text/html", page);
}

void showConfigPage()
{
  uint8_t BrokerIPAddress[4];
  int     BrokerPort;
  String  sIpAddr;
  String Password;
  String st;
  
  int nNetworks = WiFi.scanNetworks();

  st = "<select name=\"network\">";
  for (int i = 0; i < nNetworks; ++i)
    {
      st+="<option value=\""; st+= WiFi.SSID(i);st+="\""; 
      Serial.print(WiFi.SSID(i));Serial.print(":");Serial.println(WiFi.SSID());
      if (WiFi.SSID(i) == WiFi.SSID())
      {
        st+="selected";
      }
      
      st+="> "; st+= WiFi.SSID(i);st+=" (";st+=WiFi.RSSI(i); st+=" dBm)";st += (WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*"; st+="</option>"; 
  
    }
  st += "</select>";

  ReadBrokerIPAddress(BrokerIPAddress);
  BrokerPort = ReadBrokerPort();
  sIpAddr = String(BrokerIPAddress[0]); sIpAddr+=".";  sIpAddr += String(BrokerIPAddress[1]); sIpAddr+="."; sIpAddr += String(BrokerIPAddress[2]); sIpAddr+="."; sIpAddr += String(BrokerIPAddress[3]);
  ReadNetworkPass(Password);

  
  String page = "<h1>MySensors ESP8266 MQTT Gateway configuration page</h1>";

  page+= "<br><form action=\"/writeConfig\" method=\"get\">";

  page+= "<style> table, th, td { border: 1px solid black;}</style>";
  page+="<table style=\"width:600\">";
  page+="<tr><th>Item</th><th>Value</th></tr>";
  page+="<tr><td>Mqtt broker ip address</td><td><input type=\"text\" name=\"ip\" value=\""; page+=sIpAddr;page+="\" size=\"20\" ></td></tr>";
  page+="<tr><td>Mqtt broker port</td><td><input type=\"text\" name=\"port\" value=\""; page+= String(BrokerPort); page+="\"size=\"20\" ></td></tr>";
  page+="<tr><td>Wireless network</td><td>";page+=st;page+="</td></tr>";
  page+="<tr><td>Wireless password</td><td><input type=\"text\" name = \"pw\" value=\""; page+=Password;page+="\" size=\"20\"></td></tr>";
  page+="</table>";

 
  page += "Save configuration: <input type=\"submit\" value=\"Save\">";
  page += "</form>";
  
  
  WebServer.send(200, "text/html", page);
  

}

void storeConfigData(String Ip, String Port, String Network, String Pass)
{
  Serial.println("-- Storing configuration: ");
  Serial.print("Broker ip address: "); Serial.println(Ip);
  Serial.print("Broker port:       "); Serial.println(Port);
  Serial.print("Network name:      "); Serial.println(Network);
  Serial.print("Password:          "); Serial.println(Pass);

  uint8_t BrokerIp[4];
  uint16_t BrokerPort=0;

  if ((ParseIPAddress(Ip, BrokerIp) &&
      ((BrokerPort = Port.toInt()) != 0)))
  {
    WriteBrokerIP(BrokerIp);
    WriteBrokerPort(BrokerPort);
    WriteNetworkName(Network);
    WriteNetworkPass(Pass);
    WriteMagicByte();
  }
  
  showStoredPage();

}
void showStoredPage()
{
  String page = "Configuration stored, gateway will now restart, after 5 seconds";
   WebServer.send(200, "text/html", page);
   delay(5000);

   ESP.restart();
 
}

bool ParseIPAddress(String Ip, uint8_t* pBrokerIP)
{
    uint16_t acc = 0; // Accumulator
    uint8_t dots = 0;
    char IpArray[20];
    char* pIp = IpArray;

    Ip.toCharArray(pIp, 20);

    while (*pIp)
    {
        char c = *pIp++;
        if (c >= '0' && c <= '9')
        {
            acc = acc * 10 + (c - '0');
            if (acc > 255) {
                // Value out of [0..255] range
                return false;
            }
        }
        else if (c == '.')
        {
            if (dots == 3) {
                // Too much dots (there must be 3 dots)
                return false;
            }
            pBrokerIP[dots++] = acc;
            acc = 0;
        }
        else
        {
            // Invalid char
            return false;
        }
    }

    if (dots != 3) {
        // Too few dots (there must be 3 dots)
        return false;
    }
    pBrokerIP[3] = acc;
    return true;
  
}

