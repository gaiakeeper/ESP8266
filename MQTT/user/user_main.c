/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *     2014/1/1, v1.0 create this file.
*******************************************************************************/
#include "ets_sys.h"
#include "uart.h"

#include "osapi.h"
#include "ip_addr.h"
#include "espconn.h"
#include "mem.h"

#include "user_interface.h"
#include "smartconfig.h"

#include "mqtt.h"
#include "config.h"

#define MQTT_HOST			"iot.eclipse.org" //or "mqtt.yourdomain.com"
#define MQTT_PORT			1883

#define MQTT_CLIENT_ID		"DVES_%08X"
#define MQTT_USER			"DVES_USER"
#define MQTT_PASS			"DVES_PASS"

#define MQTT_KEEPALIVE		120	 /*second*/
#define MQTT_SECURITY		0

typedef struct {
	uint32_t cfg_holder;			// should be in SYSCFG
	uint8_t device_id[16];			// should be in SYSCFG

// add to load/save configuration
	struct station_config stationConf;

	uint8_t mqtt_host[64];
	uint32_t mqtt_port;
	uint8_t mqtt_user[32];
	uint8_t mqtt_pass[32];

	uint8_t mqtt_keepalive;
	uint8_t mqtt_security;
} SYSCFG;
SYSCFG sysCfg;

void ICACHE_FLASH_ATTR CFG_Init()
{
	os_memset(&sysCfg, 0x00, sizeof sysCfg);
	sysCfg.cfg_holder = CFG_HOLDER;

	// default configuration
	os_memset(&sysCfg.stationConf, 0, sizeof(struct station_config));

	os_sprintf(sysCfg.device_id, MQTT_CLIENT_ID, system_get_chip_id());
	os_sprintf(sysCfg.mqtt_host, "%s", MQTT_HOST);
	sysCfg.mqtt_port = MQTT_PORT;
	os_sprintf(sysCfg.mqtt_user, "%s", MQTT_USER);
	os_sprintf(sysCfg.mqtt_pass, "%s", MQTT_PASS);

	sysCfg.mqtt_keepalive = MQTT_KEEPALIVE;
	sysCfg.mqtt_security = MQTT_SECURITY;

	os_printf("default configuration\n");

}


MQTT_Client mqttClient;


void mqttConnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	os_printf("MQTT: Connected\r\n");

	MQTT_Subscribe(client, "/sugar_home/air_conditioner/2/R", 2);
	MQTT_Subscribe(client, "/sugar_home/air_conditioner/4/R", 2);

	MQTT_Publish(client, "/sugar_home/air_conditioner/2", "on", 2, 2, 0);
	MQTT_Publish(client, "/sugar_home/air_conditioner/3", "28", 2, 2, 0);
	MQTT_Publish(client, "/sugar_home/air_conditioner/4", "24", 2, 2, 0);
}

void mqttDisconnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	os_printf("MQTT: Disconnected\r\n");
}

void mqttPublishedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	os_printf("MQTT: Published\r\n");
}

void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
	char *topicBuf = (char*)os_zalloc(topic_len+1),
			*dataBuf = (char*)os_zalloc(data_len+1);

	MQTT_Client* client = (MQTT_Client*)args;

	os_memcpy(topicBuf, topic, topic_len);
	topicBuf[topic_len] = 0;

	os_memcpy(dataBuf, data, data_len);
	dataBuf[data_len] = 0;

	os_printf("Receive topic: %s, data: %s \r\n", topicBuf, dataBuf);
	os_free(topicBuf);
	os_free(dataBuf);
}

void ICACHE_FLASH_ATTR
smartconfig_done(sc_status status, void *pdata)
{
    switch(status) {
        case SC_STATUS_WAIT:
            os_printf("SC_STATUS_WAIT\n");
            break;
        case SC_STATUS_FIND_CHANNEL:
            os_printf("SC_STATUS_FIND_CHANNEL\n");
            break;
        case SC_STATUS_GETTING_SSID_PSWD:
            os_printf("SC_STATUS_GETTING_SSID_PSWD\n");
			sc_type *type = pdata;
            if (*type == SC_TYPE_ESPTOUCH) {
                os_printf("SC_TYPE:SC_TYPE_ESPTOUCH\n");
            } else {
                os_printf("SC_TYPE:SC_TYPE_AIRKISS\n");
            }
            break;
        case SC_STATUS_LINK:
            os_printf("SC_STATUS_LINK\n");
    		os_memcpy(&(sysCfg.stationConf), pdata, sizeof(struct station_config));
    		CFG_Save((uint32*)&sysCfg, sizeof(sysCfg));		// save to the next

    		wifi_station_set_config(&(sysCfg.stationConf));
        	wifi_station_connect();
            break;
        case SC_STATUS_LINK_OVER:
            os_printf("SC_STATUS_LINK_OVER\n");
            if (pdata != NULL) {
				//SC_TYPE_ESPTOUCH
                uint8 phone_ip[4] = {0};

                os_memcpy(phone_ip, (uint8*)pdata, 4);
                os_printf("Phone ip: %d.%d.%d.%d\n",phone_ip[0],phone_ip[1],phone_ip[2],phone_ip[3]);
            }
            smartconfig_stop();
            break;
    }
}

void ICACHE_FLASH_ATTR
wifi_handle_event_cb(System_Event_t *evt)
{
	os_printf("event %x\n", evt->event);
	switch (evt->event) {
		case EVENT_STAMODE_CONNECTED:
			os_printf("connect to ssid %s, channel %d\n",
						evt->event_info.connected.ssid,
						evt->event_info.connected.channel);
			break;
		case EVENT_STAMODE_DISCONNECTED:
			os_printf("disconnect from ssid %s, reason %d\n",
						evt->event_info.disconnected.ssid,
						evt->event_info.disconnected.reason);
/*
			// Connection Error --> reset AP information for smartconfig
			CFG_Reset();
*/
			//  Connection Error --> wait for smartconfig
			smartconfig_set_type(SC_TYPE_ESPTOUCH);
			smartconfig_start(smartconfig_done);
			break;
		case EVENT_STAMODE_AUTHMODE_CHANGE:
			os_printf("mode: %d -> %d\n",
						evt->event_info.auth_change.old_mode,
						evt->event_info.auth_change.new_mode);
			break;
		case EVENT_STAMODE_GOT_IP:
			os_printf("ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR,
						IP2STR(&evt->event_info.got_ip.ip),
						IP2STR(&evt->event_info.got_ip.mask),
						IP2STR(&evt->event_info.got_ip.gw));
			os_printf("\n");
			MQTT_Connect(&mqttClient);
			break;
		case EVENT_SOFTAPMODE_STACONNECTED:
			os_printf("station: " MACSTR "join, AID = %d\n",
						MAC2STR(evt->event_info.sta_connected.mac),
						evt->event_info.sta_connected.aid);
			break;
		case EVENT_SOFTAPMODE_STADISCONNECTED:
			os_printf("station: " MACSTR "leave, AID = %d\n",
						MAC2STR(evt->event_info.sta_disconnected.mac),
						evt->event_info.sta_disconnected.aid);
			break;
		default:
			break;
	}
}

void user_rf_pre_init(void)
{
}

void user_init(void)
{
	// initialize serial port for DEBUG
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	os_delay_us(1000000);

	CFG_Load((uint32*)&sysCfg, sizeof(sysCfg));
    os_printf("SDK version:%s\n", system_get_sdk_version());

	MQTT_InitConnection(&mqttClient, sysCfg.mqtt_host, sysCfg.mqtt_port, sysCfg.mqtt_security);
	MQTT_InitClient(&mqttClient, sysCfg.device_id, sysCfg.mqtt_user, sysCfg.mqtt_pass, sysCfg.mqtt_keepalive, 1);
	MQTT_InitLWT(&mqttClient, "/lwt", "offline", 0, 0);

	MQTT_OnConnected(&mqttClient, mqttConnectedCb);
	MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);
	MQTT_OnPublished(&mqttClient, mqttPublishedCb);
	MQTT_OnData(&mqttClient, mqttDataCb);

    // try to connect with the previous configuration
	wifi_set_opmode(STATION_MODE);
    wifi_set_event_handler_cb(wifi_handle_event_cb);

    if(sysCfg.stationConf.ssid[0]){
    	os_printf("CONNECT To %s\n", sysCfg.stationConf.ssid);
    	wifi_station_set_config(&(sysCfg.stationConf));
    	wifi_station_connect();
    }
    else {
		// wait for smart config
		smartconfig_set_type(SC_TYPE_ESPTOUCH);
		smartconfig_start(smartconfig_done);
    }
}
