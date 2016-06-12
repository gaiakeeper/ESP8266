/*
  ThingPlug Arduino starter kit
  v0.1
  2016-05-17
*/
/* ThingPlug on ESP8266 */
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include <tinyxml2.h>
using namespace tinyxml2;

// Update these with values suitable for your network.
const char* ssid     = "SK_WiFi8921";
const char* password = "1408044211";

/*
const char* ssid     = "dream_guest";
const char* password = "dreamguest123";
*/

// device ID, should be unique
const char* deviceID = "0.2.481.1.101.01037542879";      // change device ID with your phone number
const char* passcode = "12345";                          // 12345 for starter kit

const char* containerName = "myContainer";
const char* mgmtCmd = "myMGMT";


// MQTT configuration for SKT thingplug
const char* mqttBroker PROGMEM  = "onem2m.sktiot.com";
const int mqttBrokerPORT        = 1883;

void mqttCallback(char* topic, byte* payload, unsigned int plength);


// WiFi on ESP8266 and MQTT for SKT thingplug
WiFiClient wifiClient;
PubSubClient mqttClient(mqttBroker, mqttBrokerPORT, mqttCallback, wifiClient);


#define MAX_BUFFER_SIZE     1024
char buf[MAX_BUFFER_SIZE+1];

char mqttPubTopic[64];
char nodeID[24];       // 22bytes
char deviceKey[90];    // 88bytes

boolean TP_subscribe()
{
  // subscribe to response from thing plug server
  sprintf(buf, "/oneM2M/resp/%s/+", deviceID);
  if(mqttClient.subscribe(buf)){
    Serial.print("Subscribed to "); Serial.println(buf);
  }
  else {
    Serial.print("Failed to subscribe to "); Serial.println(buf);
    return false;
  }

  // subscribe to request from application
  sprintf(buf, "/oneM2M/req/+/%s", deviceID);
  if(mqttClient.subscribe(buf)){
    Serial.print("Subscribed to "); Serial.println(buf);
  }
  else {
    Serial.print("Failed to subscribe to "); Serial.println(buf);
    return false;
  }

  return false;
}

boolean TP_publish(const char* payload, unsigned int plength, boolean retained)
{
  Serial.print("[PUBLISH] ");
  for (int i = 0; i < plength; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  return mqttClient.publish(mqttPubTopic, (unsigned char*)payload, plength, retained);
}

const char* m2mBEGIN PROGMEM = "<m2m:req xmlns:m2m=\"http://www.onem2m.org/xml/protocols\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.onem2m.org/xml/protocols CDT-requestPrimitive-v1_0_0.xsd\">";

// const unsigned char mqttPubCreateNode[] PROGMEM = "<m2m:req xmlns:m2m=\"http://www.onem2m.org/xml/protocols\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.onem2m.org/xml/protocols CDT-requestPrimitive-v1_0_0.xsd\"><op>1</op><to>/ThingPlug</to><fr>0.2.481.1.101.01037542879</fr><ty>14</ty><ri>1234</ri><nm>0.2.481.1.101.01037542879</nm><cty>application/vnd.onem2m-prsp+xml</cty><pc><nod><ni>0.2.481.1.101.01037542879</ni></nod></pc></m2m:req>";
//*<ri>ND00000000000000001032</ri> 값 parsing 필요 -> remoteCSE Create할 때, <nl>값으로 설정필요
//* <fr><ni>의 0.2.481.1.101.01037542879 값 사용자 정의 필요(ThingPlug OID)
bool TP_createNode()
{
  sprintf(buf, "%s<op>1</op><to>/ThingPlug</to><fr>%s</fr><ty>14</ty><ri>1234</ri><nm>%s</nm><cty>application/vnd.onem2m-prsp+xml</cty>"
               "<pc><nod><ni>%s</ni></nod></pc></m2m:req>",
    m2mBEGIN, deviceID, deviceID, deviceID);
    
  return TP_publish(buf, strlen(buf), true);  
}

// const unsigned char mqttPubCreateRemoteCSE[] PROGMEM = "<m2m:req xmlns:m2m=\"http://www.onem2m.org/xml/protocols\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.onem2m.org/xml/protocols CDT-requestPrimitive-v1_0_0.xsd\"><op>1</op><to>/ThingPlug</to><fr>0.2.481.1.101.01037542879</fr><ty>16</ty><ri>1234</ri><passCode>12345</passCode><cty>application/vnd.onem2m-prsp+xml</cty><pc><csr><cst>3</cst><csi>0.2.481.1.101.01037542879</csi><poa>MQTT|0.2.481.1.101.01037542879</poa><rr>true</rr><nl>ND00000000000000001032</nl></csr></pc></m2m:req>";
//* <passCode>12345</passCode> 값 사용자 정의 필요
//* <fr><csi><poa>의 0.2.481.1.101.01037542879 값 사용자 정의 필요(ThingPlug OID)
//* Node Create후 response의 <ri>ND00000000000000001032</ri> 값을 <nl>값으로 설정필요
//* <dKey>64bit based decoding value</dKey> 값 parsing 필요 -> 하위 Create들 실행시 필요
bool TP_createRemoteCSE()
{
  sprintf(buf, "%s<op>1</op><to>/ThingPlug</to><fr>%s</fr><nm>%s</nm><ty>16</ty><ri>1234</ri><passCode>%s</passCode><cty>application/vnd.onem2m-prsp+xml</cty>"
               "<pc><csr><cst>3</cst><csi>%s</csi><poa>MQTT|%s</poa><rr>true</rr><nl>%s</nl></csr></pc></m2m:req>",
    m2mBEGIN, deviceID, deviceID, passcode, deviceID, deviceID, nodeID);
    
  return TP_publish(buf, strlen(buf), true);  
}

// const unsigned char mqttPubCreateContainer[] PROGMEM = "<m2m:req xmlns:m2m=\"http://www.onem2m.org/xml/protocols\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.onem2m.org/xml/protocols CDT-requestPrimitive-v1_0_0.xsd\"><op>1</op><to>/ThingPlug/remoteCSE-0.2.481.1.101.01037542879</to><fr>0.2.481.1.101.01037542879</fr><ty>3</ty><ri>1234</ri><nm>myContainer</nm><dKey>SU1VRWc4SjRkT1NNRmhwdEdTR3h6OERQdmlCdGdiV05oczZiblZKQ285NDJybmd4clVKVkNSQ3lLRkpnUmtGbQ==</dKey><cty>application/vnd.onem2m-prsp+xml</cty><pc><cnt><lbl>con</lbl></cnt></pc></m2m:req>";
//* <nm>myContainer</nm> 값 사용자 정의 필요
//* <fr><ni>의 0.2.481.1.101.01037542879 값 사용자 정의 필요(ThingPlug OID)
//* RemoteCSE Create후 response의 <dKey>64bit based decoding value</dKey> 값 설정필요
bool TP_createContainer()
{
  sprintf(buf, "%s<op>1</op><to>/ThingPlug/remoteCSE-%s</to><fr>%s</fr><ty>3</ty><ri>1234</ri><nm>%s</nm><dKey>%s</dKey><cty>application/vnd.onem2m-prsp+xml</cty>"
               "<pc><cnt><lbl>con</lbl></cnt></pc></m2m:req>",
    m2mBEGIN, deviceID, deviceID, containerName, deviceKey);
    
  return TP_publish(buf, strlen(buf), true);  
}

// const unsigned char mqttPubCreateMgmtCmd[] PROGMEM = "<m2m:req xmlns:m2m=\"http://www.onem2m.org/xml/protocols\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.onem2m.org/xml/protocols CDT-requestPrimitive-v1_0_0.xsd\"><op>1</op><to>/ThingPlug</to><fr>0.2.481.1.101.01037542879</fr><ty>12</ty><ri>1234</ri><nm>myMGMT0.2.481.1.101.01037542879</nm> <dKey>SU1VRWc4SjRkT1NNRmhwdEdTR3h6OERQdmlCdGdiV05oczZiblZKQ285NDJybmd4clVKVkNSQ3lLRkpnUmtGbQ==</dKey><cty>application/vnd.onem2m-prsp+xml</cty><pc><mgc><cmt>sensor_1</cmt><exe>false</exe><ext>ND00000000000000001032</ext></mgc></pc></m2m:req>";
//* <nm>myMGMT0.2.481.1.101.01037542879</nm> 값 사용자 정의 필요
//* <cmt>sensor_1</cmt>  값 사용자 정의 필요
//* Node Create후 response의 <ri>ND00000000000000001032</ri> 값을 <ext>값으로 설정필요
//* RemoteCSE Create후 response의 <dKey>64bit based decoding value</dKey> 값 설정필요
bool TP_createMgmtCmd()
{
  sprintf(buf, "%s<op>1</op><to>/ThingPlug</to><fr>%s</fr><ty>12</ty><ri>1234</ri><nm>%s.%s</nm><dKey>%s</dKey><cty>application/vnd.onem2m-prsp+xml</cty>"
               "<pc><mgc><cmt>sensor_1</cmt><exe>false</exe><ext>%s</ext></mgc></pc></m2m:req>",
    m2mBEGIN, deviceID, mgmtCmd, deviceID, deviceKey, nodeID);
    
  return TP_publish(buf, strlen(buf), true);  
}

// const unsigned char mqttPubCreateContentInstance[] PROGMEM = "<m2m:req xmlns:m2m=\"http://www.onem2m.org/xml/protocols\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.onem2m.org/xml/protocols CDT-requestPrimitive-v1_0_0.xsd\"><op>1</op><to>/ThingPlug/remoteCSE-0.2.481.1.101.01037542879/container-myContainer</to><fr>0.2.481.1.101.01037542879</fr><ty>4</ty><ri>1234</ri><cty>application/vnd.onem2m-prsp+xml</cty> <dKey>SU1VRWc4SjRkT1NNRmhwdEdTR3h6OERQdmlCdGdiV05oczZiblZKQ285NDJybmd4clVKVkNSQ3lLRkpnUmtGbQ==</dKey><pc><cin><cnf>text</cnf><con>45</con></cin></pc></m2m:req>";
//* <to>/ThingPlug/remoteCSE-0.2.481.1.101.01037542879/container-myContainer</to> 의 container 이름 값 설정필요
//* <cnf>text</cnf>, <con>45</con> 값들 사용자 정의 필요
//* RemoteCSE Create후 response의 <dKey>64bit based decoding value</dKey> 값 설정필요
bool TP_createContentInstance()
{
  sprintf(buf, "%s<op>1</op><to>/ThingPlug/remoteCSE-%s/container-%s</to><fr>%s</fr><ty>4</ty><ri>1234</ri><cty>application/vnd.onem2m-prsp+xml</cty><dKey>%s</dKey>"
               "<pc><cin><cnf>text</cnf><con>%d</con></cin></pc></m2m:req>",
    m2mBEGIN, deviceID, containerName, deviceID, deviceKey, random(25, 45));
    
  return TP_publish(buf, strlen(buf), true);  
}

void setupSerial() {
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
}

void setupWiFi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

boolean setupThingPlug()
{
  Serial.println("Connecting to Thing Plug Server...");
  if (mqttClient.connect(deviceID)) {
    Serial.println("Thing Plug Server connected");
    TP_subscribe();
  }
  else Serial.println("Try connecting to Thing Plug Server...");
    
  sprintf(mqttPubTopic, "/oneM2M/req/%s/ThingPlug", deviceID);
}

enum PROCESS {
  CREATE_NODE,
  CREATE_REMOTE_CSE,
  CREATE_CONTAINER,
  CREATE_MGMT_CMD,
  CREATE_CONTENT_INSTANCE
};
int state = CREATE_NODE;

XMLDocument xml;
void mqttCallback(char* topic, byte* payload, unsigned int plength) {
  Serial.print("[RESPONSE] ");
  for (int i = 0; i < plength; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
    
  xml.Clear();
  xml.Parse((char*)payload, plength);
  XMLElement* root = xml.RootElement();

  XMLElement* dkey = root->FirstChildElement("dKey");
  if (dkey){
    strcpy(deviceKey, dkey->GetText());
  }
  
  XMLElement* pc = xml.RootElement()->FirstChildElement("pc");
  XMLElement* next = pc->FirstChildElement();
  if (next==NULL){
    Serial.println("NO CONTENT");
  }
  else if (strcmp(next->Name(), "nod") == 0){
    XMLElement* ri = next->FirstChildElement("ri");
    if(ri){
      strcpy(nodeID, ri->GetText());
      state = CREATE_REMOTE_CSE;        // goto next
    }
  }
  else if (strcmp(next->Name(), "csr") == 0){
    state = CREATE_CONTAINER;        // goto next
  }
  else if (strcmp(next->Name(), "cnt") == 0){
    state = CREATE_MGMT_CMD;          // goto next
  }
  else if (strcmp(next->Name(), "mgc") == 0){
    state = CREATE_CONTENT_INSTANCE;  // goto next
  }
  else if (strcmp(next->Name(), "cin") == 0){
  }
}

void CreateLoop(){
  switch(state){
    case CREATE_NODE: TP_createNode(); break;
    case CREATE_REMOTE_CSE: TP_createRemoteCSE(); break;
    case CREATE_CONTAINER: TP_createContainer(); break;
    case CREATE_MGMT_CMD: TP_createMgmtCmd(); break;
    case CREATE_CONTENT_INSTANCE: TP_createContentInstance(); break;
    default: break;
  }
  delay(1000);
}

void setup()
{
  randomSeed(analogRead(0));
  
  setupSerial();
  setupWiFi();
  setupThingPlug();
}

void loop()
{
  mqttClient.loop();
  CreateLoop();
}

