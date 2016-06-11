# ThingPlug_Arduino
This is an example ThingPlug device on ESP8266. This is derived from [SKT-ThingPlug starter kit](https://github.com/SKT-ThingPlug/thingplug-starter-kit).

# Required Libraries
This requires MQTT(PubSubClient) and TinyXML2. PubSubClient can be installed using Arduino Library Manager. In installing PubSubClient, you should change the buffer size in PubSubClient.h as below.

```
// MQTT_VERSION : Pick the version
//#define MQTT_VERSION MQTT_VERSION_3_1
#ifndef MQTT_VERSION
#define MQTT_VERSION MQTT_VERSION_3_1_1
#endif

// MQTT_MAX_PACKET_SIZE : Maximum packet size
#ifndef MQTT_MAX_PACKET_SIZE
#define MQTT_MAX_PACKET_SIZE 1024
#endif

// MQTT_KEEPALIVE : keepAlive interval in Seconds
#ifndef MQTT_KEEPALIVE
#define MQTT_KEEPALIVE 60
#endif

// MQTT_SOCKET_TIMEOUT: socket timeout interval in Seconds
#ifndef MQTT_SOCKET_TIMEOUT
#define MQTT_SOCKET_TIMEOUT 30
#endif
```

TinyXML2 should be installed manually. You have only to download [tinyxml2.h/cpp](https://github.com/leethomason/tinyxml2) and copy them into Arduino Library folder with new TinyXML2 folder.

# Required Configrations
You should specify SSID/PASSWORD for Wi-Fi AP.
```
const char* ssid     = "<<<ssid>>>";
const char* password = "<<<password>>>";
```

And next, you should create your own device ID. Phone number is recommended for the unique.
```
const char* deviceID = "0.2.481.1.101.01099889911";
```