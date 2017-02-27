#include <Time.h>
#include <TimeLib.h>


#include <ESP8266WiFi.h>
#include <WiFiUdp.h>



unsigned long sendNTPpacket(IPAddress& address);
bool SendNtpRequest();
bool WaitForNtpResponse();
bool HandleNtpResponse();
bool WaitForNewRequest();
int dstOffset (unsigned long unixTime);

unsigned int localPort = 2390;      // local port to listen for UDP packets

#define TZ_CORRECTION (3600)        // UTC + 1
#define MAX_WAIT_FOR_NTP_RESPONSE (5000)
#define TIME_BETWEEN_NTP_REQUESTS (3600000)   // Every hour
//#define TIME_BETWEEN_NTP_REQUESTS (10000)

/* Don't hardwire the IP address or we won't get the benefits of the pool.
 *  Lookup the IP address for the host name instead */
//IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "ntp0.nl.net";

unsigned long TimeAtNtpRequest = 0;
unsigned long MillisAtNtpRequest = 0;
unsigned long TimeAtStart = 0;
unsigned long TimeAtStartWaitForRequest = 0;

unsigned long ntpPacketSent = 0;
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

typedef enum
{
  NTP_WAIT_FOR_REQUEST_TIMEOUT,
  NTP_SEND_REQUEST,
  NTP_WAIT_FOR_RESPONSE,
  NTP_HANDLE_RESPONSE
} NtpStateEnum;

NtpStateEnum NtpState = NTP_SEND_REQUEST;


void setup_ntp()
{
  udp.begin(localPort);
  Serial.print("Local UDP port for NTP: ");
  Serial.println(udp.localPort());
}

void loopNtp()
{
    switch (NtpState)
    {
      case NTP_WAIT_FOR_REQUEST_TIMEOUT:
        if (WaitForNewRequest())
        {
          NtpState = NTP_SEND_REQUEST;
        }
      break;
      case NTP_SEND_REQUEST:
        if(SendNtpRequest())
        {
          NtpState = NTP_WAIT_FOR_RESPONSE;
        }
      break;
      case NTP_WAIT_FOR_RESPONSE:
        if (WaitForNtpResponse())
        {
          NtpState = NTP_HANDLE_RESPONSE;
        }
      break;
      case NTP_HANDLE_RESPONSE:
        if (HandleNtpResponse())
        {
          NtpState = NTP_WAIT_FOR_REQUEST_TIMEOUT; 
        }
      break;
    };
  
}

unsigned long getCurrentSeconds()
{
  return (time_t)((millis() - MillisAtNtpRequest) / 1000) + TimeAtNtpRequest ;
}

String getFormattedTime()
{
  unsigned long Currentseconds = ((millis() - MillisAtNtpRequest) / 1000) + TimeAtNtpRequest ;
  char outputString[20];

  sprintf(outputString, "%04d-%02d-%02d %02d:%02d:%02d", year(Currentseconds), month(Currentseconds), day(Currentseconds),
                                                   hour(Currentseconds), minute(Currentseconds), second(Currentseconds));

  return String(outputString);
  
}

String getStartTime()
{
  char outputString[20];

  sprintf(outputString, "%04d-%02d-%02d %02d:%02d:%02d", year(TimeAtStart), month(TimeAtStart), day(TimeAtStart),
                                                   hour(TimeAtStart), minute(TimeAtStart), second(TimeAtStart));

  return String(outputString);
  
}

unsigned long getTime()
{
  unsigned long Currentseconds = ((millis() - MillisAtNtpRequest) / 1000) + TimeAtNtpRequest ;

  return Currentseconds;
}

String formatTime(unsigned long timestamp)
{
  char outputString[20];

  sprintf(outputString, "%04d-%02d-%02d %02d:%02d:%02d", year(timestamp), month(timestamp), day(timestamp),
                                                   hour(timestamp), minute(timestamp), second(timestamp));

  return String(outputString);
  
}
bool WaitForNewRequest()
{
  bool bRet = false;
  if (TimeAtStartWaitForRequest == 0 )
  {
    TimeAtStartWaitForRequest = millis();
  }
  else
  {
    if (millis() > (TimeAtStartWaitForRequest + TIME_BETWEEN_NTP_REQUESTS))
    {
      TimeAtStartWaitForRequest = 0;
      bRet = true;
    }
  }
  return bRet;
}

bool SendNtpRequest()
{
  Serial.println("Started: SendNtpRequest");

  //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP); 

  sendNTPpacket(timeServerIP); // send an NTP packet to a time server

  ntpPacketSent = millis();
  NtpState = NTP_WAIT_FOR_RESPONSE;

  Serial.println("Done: SendNtpRequest");

  return true;
}

bool WaitForNtpResponse()
{
  bool bRet = false;
  Serial.println("Started: WaitForNtpResponse()");
  
  int cb = udp.parsePacket();
  if (!cb) 
  {
    if (millis() > ntpPacketSent + MAX_WAIT_FOR_NTP_RESPONSE)
    {
       Serial.println("no NTP received in time... Retrying");
       NtpState = NTP_SEND_REQUEST;     
    }
  }
  else
  {
    Serial.println("NTP response received!");    
    bRet = true;
  }
  Serial.println("Done: WaitForNtpResponse()");
  
  return bRet;
  
}

bool HandleNtpResponse()
{
  Serial.println("Started: HandleNtpResponse()");
  // We've received a packet, read the data from it
  udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

  //the timestamp starts at byte 40 of the received packet and is four bytes,
  // or two words, long. First, esxtract the two words:

  unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
  unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
  // combine the four bytes (two words) into a long integer
  // this is NTP time (seconds since Jan 1 1900):
  unsigned long secsSince1900 = highWord << 16 | lowWord;

  // now convert NTP time into everyday time:
  // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
  const unsigned long seventyYears = 2208988800UL;
  // subtract seventy years:
  unsigned long epoch = secsSince1900 - seventyYears;
  epoch += dstOffset(epoch) + TZ_CORRECTION;
  // print Unix time:
  TimeAtNtpRequest = epoch;
  MillisAtNtpRequest = millis();
  if (TimeAtStart == 0)
  {
    TimeAtStart = epoch;
  }
  Serial.println("Done: HandleNtpResponse()");
  return true;
  
}

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

int dstOffset (unsigned long unixTime)
{
  //Receives unix epoch time and returns seconds of offset for local DST
  //Valid thru 2099 for US only, Calculations from "http://www.webexhibits.org/daylightsaving/i.html"
  //Code idea from jm_wsb @ "http://forum.arduino.cc/index.php/topic,40286.0.html"
  //Get epoch times @ "http://www.epochconverter.com/" for testing
  //DST update wont be reflected until the next time sync
  time_t t = unixTime;
  int beginDSTDay = (31 - (5 * year(t)/4 + 4) % 7);
  int beginDSTMonth=3;
  int endDSTDay = (31 - (5 * year(t) / 4+ 1) % 7);     
  int endDSTMonth=10;
  if (((month(t) > beginDSTMonth) && (month(t) < endDSTMonth))
    || ((month(t) == beginDSTMonth) && (day(t) > beginDSTDay))
    || ((month(t) == beginDSTMonth) && (day(t) == beginDSTDay) && (hour(t) >= 1))
    || ((month(t) == endDSTMonth) && (day(t) < endDSTDay))
    || ((month(t) == endDSTMonth) && (day(t) == endDSTDay) && (hour(t) < 1)))
    return (3600);  //Add back in one hours worth of seconds - DST in effect
  else
    return (0);  //NonDST
}
