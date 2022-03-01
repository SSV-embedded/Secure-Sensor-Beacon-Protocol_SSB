#include "webserver.h"
#include "html_page.h"
#include <WiFi.h>
#include <WiFiAP.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

  extern AsyncWebServer server;  
  extern AsyncWebSocket ws;
  extern String wlan_ssid;
  extern String wlan_pass;
  extern String mqtt_server_ip;
  extern boolean auto_config;
  extern const char *WLAN_AP_SSID;
  extern const char *WLAN_AP_PASSWORD;
  extern String currentWiFiConfig;


//send inputstring to every connected client
void notifyClients(String &str)
{
   ws.textAll( str );
}


void initWebSocket()
{
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}


String processor(const String& var)
{
  return "";
}


void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) 
{
  switch (type) { 
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      notifyClients(currentWiFiConfig);
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
   
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:  
      break;
  }
}


/*Take the incoming configurations and split 'em up into fitting parts*/
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    int iterator = 0;
    //seperate the incoming message NUMBER , NUMBER
    //search for the dividing komma in the string 
    //seperator holds the index of the current komma
   
    /*---GET THE SSID---*/
    while( (char)data[iterator] != ','){
      wlan_ssid += (char)data[iterator];
      iterator += 1;
    }       
    iterator += 1;
    
    /*---GET THE PASSWORD---*/
    while( (char)data[iterator] != ','){
      wlan_pass += (char)data[iterator];
      iterator += 1;
    }      
    iterator += 1;
   String config_type = "";
    /*---GET THE CONFIGURATION TYPE---*/
    while( (char)data[iterator] != ',' && iterator < len){
      config_type += (char)data[iterator];
      iterator += 1;
    }      

    if( config_type == "auto" ){
      //configuration shall use the mdns service discovery!
      auto_config = true;
      return;
    }
    
    auto_config = false;
    //manual config
    iterator += 1;
    /*---GET THE MQTT BROKER IP---*/
    while( (char)data[iterator] != ',' && iterator < len){
      mqtt_server_ip += (char)data[iterator];
      iterator += 1;
    }        
  }
}
