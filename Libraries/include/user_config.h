#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_

// MQTT configuration
#define MQTT_BUF_SIZE			1024
#define MQTT_KEEPALIVE			120	 /*second*/

#define MQTT_RECONNECT_TIMEOUT 	5	/*second*/

#define PROTOCOL_NAMEv31	/*MQTT version 3.1 compatible with Mosquitto v0.15*/
//#define PROTOCOL_NAMEv311	/*MQTT version 3.11 compatible with https://eclipse.org/paho/clients/testing/*/

#endif // _USER_CONFIG_H_
