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

#include "osapi.h"
#include "ip_addr.h"
#include "espconn.h"
#include "mem.h"

#include "user_interface.h"
#include "smartconfig.h"

// libraries
#include "config.h"
#include "uart.h"
#include "mqtt.h"
#include "yxml2.h"

// device ID, starter kit recommend to use your phone number for uniqueness
uint8_t* deviceID		= "0.2.481.1.101.01037542879";      // change device ID with your phone number
uint8_t* passcode		= "12345";                          // 12345 for starter kit
uint8_t* containerName	= "myContainer";
uint8_t* mgmtCmd		= "myMGMT";

uint8_t* mqttBroker		= "onem2m.sktiot.com";
uint32  mqttBrokerPort	= 1883;
uint32	mqttKeepalive	= 120;		// seconds
uint8_t	mqttSecurity	= 0;
uint8_t	mqttClean		= 1;		// clean session

const int	mqttQOS			= 0;		// 2: deliver once
const int	mqttRetained	= 1;		// 1: true, 0: false

typedef struct {
	uint32_t cfg_holder;			// should be in SYSCFG
	uint8_t device_id[16];			// should be in SYSCFG

// add to load/save configuration
	struct station_config stationConf;
} SYSCFG;
SYSCFG sysCfg;

void ICACHE_FLASH_ATTR CFG_Init()
{
	os_memset(&sysCfg, 0x00, sizeof sysCfg);
	sysCfg.cfg_holder = CFG_HOLDER;

	// default configuration
	os_memset(&sysCfg.stationConf, 0, sizeof(struct station_config));
	os_printf("default configuration\n");

}

// buffer for sharing
#define MAX_BUFFER_SIZE     1024
char buf[MAX_BUFFER_SIZE+1];

char mqttPubTopic[64];
char nodeID[24];		// 22bytes for node ID
char deviceKey[90];		// 88bytes for device key

// ThingPlug create process
os_timer_t timer;		// timer
enum PROCESS {
  CREATE_NODE,
  CREATE_REMOTE_CSE,
  CREATE_CONTAINER,
  CREATE_MGMT_CMD,
  CREATE_CONTENT_INSTANCE
};
int state = CREATE_NODE;

MQTT_Client mqttClient;

// ThingPlug publish methods
const char* m2mBEGIN			= "<m2m:req xmlns:m2m=\"http://www.onem2m.org/xml/protocols\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.onem2m.org/xml/protocols CDT-requestPrimitive-v1_0_0.xsd\">";
const char* m2mCreateNode		= "%s<op>1</op><to>/ThingPlug</to><fr>%s</fr><ty>14</ty><ri>1234</ri><nm>%s</nm><cty>application/vnd.onem2m-prsp+xml</cty>"
								  "<pc><nod><ni>%s</ni></nod></pc></m2m:req>";
const char* m2mCreateRemoteCSE	= "%s<op>1</op><to>/ThingPlug</to><fr>%s</fr><nm>%s</nm><ty>16</ty><ri>1234</ri><passCode>%s</passCode><cty>application/vnd.onem2m-prsp+xml</cty>"
								  "<pc><csr><cst>3</cst><csi>%s</csi><poa>MQTT|%s</poa><rr>true</rr><nl>%s</nl></csr></pc></m2m:req>";
const char* m2mCreateContainer	= "%s<op>1</op><to>/ThingPlug/remoteCSE-%s</to><fr>%s</fr><ty>3</ty><ri>1234</ri><nm>%s</nm><dKey>%s</dKey><cty>application/vnd.onem2m-prsp+xml</cty>"
								  "<pc><cnt><lbl>con</lbl></cnt></pc></m2m:req>";
const char* m2mCreateMgmtCmd	= "%s<op>1</op><to>/ThingPlug</to><fr>%s</fr><ty>12</ty><ri>1234</ri><nm>%s.%s</nm><dKey>%s</dKey><cty>application/vnd.onem2m-prsp+xml</cty>"
								  "<pc><mgc><cmt>sensor_1</cmt><exe>false</exe><ext>%s</ext></mgc></pc></m2m:req>";
const char* m2mCreateContent	= "%s<op>1</op><to>/ThingPlug/remoteCSE-%s/container-%s</to><fr>%s</fr><ty>4</ty><ri>1234</ri><cty>application/vnd.onem2m-prsp+xml</cty><dKey>%s</dKey>"
								  "<pc><cin><cnf>text</cnf><con>%d</con></cin></pc></m2m:req>";
BOOL ICACHE_FLASH_ATTR
tpCreateNode()
{
	os_sprintf(buf, m2mCreateNode, m2mBEGIN, deviceID, deviceID, deviceID);
os_printf("tpCreateNode: %s\n", buf);
	return MQTT_Publish(&mqttClient, mqttPubTopic, buf, os_strlen(buf), mqttQOS, mqttRetained);
}

BOOL ICACHE_FLASH_ATTR
tpCreateRemoteCSE()
{
	os_sprintf(buf, m2mCreateRemoteCSE, m2mBEGIN, deviceID, deviceID, passcode, deviceID, deviceID, nodeID);
os_printf("tpCreateRemoteCSE: %s\n", buf);
	return MQTT_Publish(&mqttClient, mqttPubTopic, buf, os_strlen(buf), mqttQOS, mqttRetained);
}

BOOL ICACHE_FLASH_ATTR
tpCreateContainer()
{
	os_sprintf(buf, m2mCreateContainer, m2mBEGIN, deviceID, deviceID, containerName, deviceKey);
os_printf("tpCreateContainer: %s\n", buf);
	return MQTT_Publish(&mqttClient, mqttPubTopic, buf, os_strlen(buf), mqttQOS, mqttRetained);
}

BOOL ICACHE_FLASH_ATTR
tpCreateMgmtCmd()
{
	os_sprintf(buf, m2mCreateMgmtCmd, m2mBEGIN, deviceID, mgmtCmd, deviceID, deviceKey, nodeID);
os_printf("tpCreateMgmtCmd: %s\n", buf);
	return MQTT_Publish(&mqttClient, mqttPubTopic, buf, os_strlen(buf), mqttQOS, mqttRetained);
}

BOOL tpCreateContentInstance()
{
	os_sprintf(buf, m2mCreateContent, m2mBEGIN, deviceID, containerName, deviceID, deviceKey, 40+(os_random()%5));
os_printf("tpCreateContentInstance: %s\n", buf);
	return MQTT_Publish(&mqttClient, mqttPubTopic, buf, os_strlen(buf), mqttQOS, mqttRetained);
}


// MQTT callbacks
void ICACHE_FLASH_ATTR
mqttConnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	os_printf("MQTT: Connected\r\n");

	// subscribe to response from thing plug server
	os_sprintf(buf, "/oneM2M/resp/%s/+", deviceID);
	MQTT_Subscribe(client, buf, mqttQOS);

	// subscribe to request from application
	os_sprintf(buf, "/oneM2M/req/+/%s", deviceID);
	MQTT_Subscribe(client, buf, mqttQOS);

	// topic to publish
	os_sprintf(mqttPubTopic, "/oneM2M/req/%s/ThingPlug", deviceID);

	state = CREATE_NODE;
	os_timer_arm(&timer, 5000, 1);		// enable timer, 1000: 1sec, 1: repeated
}

void ICACHE_FLASH_ATTR
mqttDisconnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	os_printf("MQTT: Disconnected\r\n");

	os_timer_disarm(&timer);			// disable timer
}

void mqttPublishedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	os_printf("MQTT: Published\r\n");
}

void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
	/*
	{
		char *topicBuf = (char*)os_zalloc(topic_len+1),
				*dataBuf = (char*)os_zalloc(data_len+1);

		os_memcpy(topicBuf, topic, topic_len);
		topicBuf[topic_len] = 0;

		os_memcpy(dataBuf, data, data_len);
		dataBuf[data_len] = 0;

		os_printf("Receive topic: %s, data: %s \r\n", topicBuf, dataBuf);
		os_free(topicBuf);
		os_free(dataBuf);
	}
	*/

	MQTT_Client* client = (MQTT_Client*)args;

	XML_Document* doc = xmlNewDocument();
	xmlParse(doc, data, data_len);

	XML_Element* root = xmlRootElement(doc);
	XML_Element* dKey = xmlFindFirstChild(root, "dKey");
	if(dKey) os_strcpy(deviceKey, xmlValue(dKey));

	XML_Element* pc = xmlFindFirstChild(root, "pc");
	if (pc != NULL){
		XML_Element* next = xmlChild(pc);
		if(next==NULL){

		}
		else if(os_strcmp(xmlName(next), "nod")==0){
			XML_Element* ri = xmlFindFirstChild(next, "ri");
			if(ri){
				os_strcpy(nodeID, xmlValue(ri));
				os_printf("NODE CREATED - %s\n", nodeID);
				state = CREATE_REMOTE_CSE;			// goto next
			}
		}
		else if(os_strcmp(xmlName(next), "csr")==0){
			os_printf("REMOTE_CSE CREATED\n");
			state = CREATE_CONTAINER;			// goto next
		}
		else if(os_strcmp(xmlName(next), "cnt")==0){
			os_printf("CONTAINER CREATED\n");
			state = CREATE_MGMT_CMD;			// goto next
		}
		else if(os_strcmp(xmlName(next), "mgc")==0){
			os_printf("MGMT_CMD CREATED\n");
			state = CREATE_CONTENT_INSTANCE;			// goto next
		}
		else if(os_strcmp(xmlName(next), "cin")==0){
			os_printf("CONTENT_INSTANCE CREATED\n");
		}
	}

	xmlDeleteDocument(doc);
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


void device_timer_callback(void *arg)
{
	switch(state){
		case CREATE_NODE: tpCreateNode(); break;
		case CREATE_REMOTE_CSE: tpCreateRemoteCSE(); break;
		case CREATE_CONTAINER: tpCreateContainer(); break;
		case CREATE_MGMT_CMD: tpCreateMgmtCmd(); break;
		case CREATE_CONTENT_INSTANCE: tpCreateContentInstance(); break;
		default: break;
	}
}

void user_rf_pre_init(void)
{
}

void user_init(void)
{
	system_timer_reinit();			// initialize timer

	os_timer_disarm(&timer);
	os_timer_setfn(&timer, (os_timer_func_t *)device_timer_callback, NULL);

	// initialize serial port for DEBUG
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	os_delay_us(1000000);

	CFG_Load((uint32*)&sysCfg, sizeof(sysCfg));
    os_printf("SDK version:%s\n", system_get_sdk_version());

	MQTT_InitConnection(&mqttClient, mqttBroker, mqttBrokerPort, mqttSecurity);
	MQTT_InitClient(&mqttClient, deviceID, deviceID, passcode, mqttKeepalive, mqttClean);
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
