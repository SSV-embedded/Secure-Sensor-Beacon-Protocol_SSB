#ifndef MYWEBSERVER_H
#define MYWEBSERVER_H

#include "Arduino.h"
#include <WiFi.h>
#include <WiFiAP.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "html_page.h"

class CaptiveRequestHandler : public AsyncWebHandler {
public:
  CaptiveRequestHandler() {}
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request){
    //request->addInterestingHeader("ANY");
    return true;
  }

  void handleRequest(AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->print(html_main_page);
    request->send(response);
  }
};


//start up the Websocket
void initWebSocket();

void notifyClients(String &str);

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);

String processor(const String& var);


#endif//MYWEBSERVER_H
