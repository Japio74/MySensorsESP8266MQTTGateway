# MySensorsESP8266MQTTGateway
Extensive gateway for the Mysensors project. Offers a WebServer to monitor sensor status and reset sensors.

Offers the following features:
- Access point mode when no wifi configuration is stored. Just connect the MySGateway wifi network, and point your browser to 192.168.4.1
- Web page showing current status of the gateway and last sensor data.
- Web page that allows configuration of MQTT broker and Wifi settings.
- NeoPixel stick interface, gateway status is shown clearly using eight RGB leds:
  - Mode: Application, or Access point mode.
  - Wifi connection
  - Wifi signal strength
  - Led transport error indication
  - MQTT: Shows connection status, and when data is sent or received
  - Sensor Network data received
  - Sensor network data sent
  - One spare ;-)
  
  
  
