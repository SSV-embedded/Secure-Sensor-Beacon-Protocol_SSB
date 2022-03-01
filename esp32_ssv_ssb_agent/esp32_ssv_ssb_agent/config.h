#ifndef CONFIG_H
#define CONFIG_H


//uncomment the type of ssb agent you want to use (disable all types to just use the serial agent)
#define WiFi_AGENT
//#define ETH_AGENT //not implemented yet

//Restore Button --> Pin 39 on ATOM LITE or whatever you want on development board, e.g. pin 19.
//!!LOW ACTIVE!!
const int restoreButton = 19;

//WiFi config
const int WIFI_RETRY_DELAY = 300000;//milliseconds
const char *WLAN_AP_SSID = "ssb-agent-";      //"ssv-embedded.de Dev";
const char *WLAN_AP_PASSWORD = "123456789";   //"44deun23ssv";
const byte AP_DNS_PORT = 53;

//mDNS service discovery
static const char serviceName[]= "_ssb-mqtt";
static String mDNShostname = "ssb-agent-";

//Status updates
const unsigned long STATUS_INTERVAL = 300000;// ms between status messages

//MQTT settings
const char *MQTT_PREFIX_TOPIC = "ble2mqtt/device/";
const char *MQTT_GATEWAY_TOPIC = "mqtt2ble/gateway/";
const int MQTT_PORT = 1883;
const char *MQTT_USER = "";
const char *MQTT_PASS = "";

//Bluetooth LE settings
const int BLE_SCAN_TIME = 10; // seconds
//true: output every packet heard
//false: output first packet for each address heard in BLE_SCAN_TIME interval
const bool KEEP_DUPLICATES = true;

#endif//CONFIG_H
