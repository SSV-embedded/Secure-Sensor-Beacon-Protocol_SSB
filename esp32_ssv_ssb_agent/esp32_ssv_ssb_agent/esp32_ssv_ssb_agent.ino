#include <NimBLEDevice.h>
#include "BLEAdvertisedDevice_logger.h"
#include "config.h"
#include "utilities.h"

#ifdef WiFi_AGENT
  #include <PubSubClient.h>
  #include <WiFi.h>
  #include <WiFiUdp.h>
  #include <AsyncTCP.h>
  #include <ESPAsyncWebServer.h>
  #include <ESPmDNS.h>
  #include <Preferences.h>
  #include <ArduinoJson.h>
  #include <DNSServer.h>
  #include <SPIFFS.h>
  #include "webserver.h"
  #include "html_page.h"
#endif//WiFi_AGENT

NimBLEScan* pBLEScan;
NimBLEClient *pBLEClient;
static NimBLEAdvertisedDevice *connectable_ble_dev;
BLEAdvertisedDevice_logger<20> logger;

boolean is_scanning=false;
static std::string ble_mac;

#ifdef WiFi_AGENT
  AsyncWebServer server(80);  
  AsyncWebSocket ws("/ws");
  WiFiClient wifi;
  PubSubClient mqtt(wifi);

  boolean try_to_connect=false;
  boolean is_connectable=false;
  unsigned long last_wifi_check=0;
  unsigned long last_mqtt_check=0;
  
  uint8_t mac[6];
  String my_mac;
  String MQTT_SERVER="";
  String currentWiFiConfig="";
  //gets set in the mqtt receive callback
  const char* requested_ble_dev;
  const char* ble_write_data;
  const char* serviceUUID;
  const char* characteristicUUID;
  uint8_t ble_write_data_len=0;
  
  String wlan_ssid="";
  String wlan_pass="";
  String mqtt_server_ip="";
  boolean auto_config=true;
  boolean noConnection=false;




  //Called whenever a payload is received from a subscribed MQTT topic
  void mqtt_receive_callback(char* topic, byte* payload, unsigned int length)
  {
    StaticJsonDocument<256> doc;
    deserializeJson(doc, (char*) payload);
    static std::string toBinaryConvertedPayload = "";
    serviceUUID = doc["serviceUUID"];
    characteristicUUID = doc["characteristicUUID"];
    requested_ble_dev = doc["device"];
    ble_write_data = doc["data"]; 
    ble_write_data_len = strlen(ble_write_data);  
    Serial.println("Received a MQTT message!");
    Serial.print("Address is: ");Serial.println(requested_ble_dev);
    Serial.print("ServiceUUID is: ");Serial.println(serviceUUID);
    Serial.print("CharacteristicUUID is: ");Serial.println(characteristicUUID);
    Serial.print("Payload is: ");Serial.println(ble_write_data);
    Serial.print("Sizeof Payload is: ");Serial.println(ble_write_data_len);
    Serial.println("Connecting to the device at the next possible time!");  
    if(ble_write_data_len % 2 != 0){
        Serial.println("Received odd payload, unable to convert to binary hex format!");
      std::string topic = MQTT_GATEWAY_TOPIC;
      topic += ble_mac;
      topic += "/status";
  
      if (mqtt.connected()) {
        mqtt.beginPublish(topic.c_str(), topic.length(), false);
        mqtt.print("Update Failed!");
        mqtt.print("Odd length!");
        mqtt.endPublish();  //does nothing?
      }    
      //PUBLISH ERROR ON MQTT
      return;
    }
    
    //convert palyoad to advertisable binary
    toBinaryConvertedPayload = strToBinary( ble_write_data, ble_write_data_len );
    ble_write_data = toBinaryConvertedPayload.c_str();
    try_to_connect = true; 
    Serial.print("Converted payload is now: ");
    for(int i = 0; i < ble_write_data_len/2; i++){
      Serial.print(ble_write_data[i], HEX);
      Serial.print(" ");
    }
    Serial.println("");  
  }



  bool check_mqtt()
  {
    if(mqtt.connected()) { return true; }
      
    //no point to continue if no network
    if(WiFi.status() != WL_CONNECTED) { return false; }
    
    // reconnect
    Serial.print("MQTT reconnect...");   
    //Attempt to connect                                                 
    int connect_status = mqtt.connect(my_mac.c_str(), MQTT_USER, MQTT_PASS);
    if(connect_status){ 
      mqtt.subscribe((MQTT_GATEWAY_TOPIC + ble_mac).c_str());
      Serial.print("Subscribed to: ");Serial.println((MQTT_GATEWAY_TOPIC + ble_mac).c_str());
    }else{
      Serial.print("failed, rc=");
      Serial.println(mqtt.state());
    }
    return mqtt.connected();
  }



  String browseService(const char * service, const char * proto)
  {
      int n = MDNS.queryService(service, proto);
      Serial.printf("Searching for service _%s._%s.local. ... ", service, proto);   
      if (n == 0) {
          Serial.println("No services found.");
          return "";
      } else {
          Serial.print(n);
          Serial.println(" service(s) found!");
          for (int i = 0; i < n; ++i) {
              // Print details for each service found
              Serial.print("  ");
              Serial.print(i + 1);
              Serial.print(": ");
              Serial.print(MDNS.hostname(i));
              Serial.print(" (");
              Serial.print(MDNS.IP(i));
              Serial.print(":");
              Serial.print(MDNS.port(i));
              Serial.println(")");
          }    
      }  
      return MDNS.IP(0).toString();
  }



  bool check_wifi(bool nowait)
  {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.print("*");
      if ((millis() - last_wifi_check) >= WIFI_RETRY_DELAY) {
        int retries = 5;
        Serial.print("*");
        while (WiFi.status() != WL_CONNECTED) {
          WiFi.begin(wlan_ssid.c_str(), wlan_pass.c_str());
          delay(500);
          Serial.print(".");
          retries--;
          if (retries == 0) { break; }
        }
  
        if (retries == 0) {
          String msg = "WiFi: Failed, waiting for " + String(WIFI_RETRY_DELAY/1000) + " seconds";
          Serial.println(msg);
        }
  
        last_wifi_check = millis();
      } else {
        Serial.print("x");
      }
    }
    return (WiFi.status() == WL_CONNECTED);
  }



  void updateSensorBox(NimBLEAdvertisedDevice* dev,const char* serviceUUID,const char* characteristicUUID, unsigned char *payload, uint8_t len)
  {
    std::string err_state;
    std::string topic = MQTT_GATEWAY_TOPIC;
    topic += ble_mac;
    topic += "/status";
    
    pBLEClient = NimBLEDevice::createClient();
    pBLEClient->connect(connectable_ble_dev);
  
    BLEUUID servUUID(serviceUUID);
    BLEUUID charUUID(characteristicUUID);
    
    NimBLERemoteService *pRmtServ = nullptr;
    pRmtServ = pBLEClient->getService(servUUID);
  
    if(pRmtServ != nullptr){  
      NimBLERemoteCharacteristic *pRmtChar = nullptr;
      pRmtChar = pRmtServ -> getCharacteristic(charUUID);     
      if(pRmtChar != nullptr){     
        if(pRmtChar->canWrite()){
          if(pRmtChar->writeValue(payload, len, false)){     
            Serial.println("Writing successful! Finishing sensor update...");
            if (mqtt.connected()) {
              mqtt.beginPublish(topic.c_str(), topic.length(), false);
              mqtt.print("Finished sensor update on: ");
              mqtt.print(connectable_ble_dev->getAddress().toString().c_str());
              mqtt.endPublish();  // does nothing?
            }    
              Serial.println("Disconnecting!");
              pBLEClient -> disconnect();
            }else{
            Serial.println("An arror occured during write process! Writing to sensor failed! Exiting sensor update...");
            err_state = "Write Error!";
          }
        }else{
          Serial.println("No write permission to characteristic! Aborting sensor update...");   
          err_state = "No write permission!";
        }    
      }else{
        Serial.println("Unable to get characteristic! Aborting sensor update...");
        err_state = "Characteristic not found!";
      }
    }else{
      Serial.println("Unable to get service! Aborting sensor update...");
      err_state = "Service not found!";
    }
  
    Serial.println("Disconnecting!");
    pBLEClient -> disconnect();
    
    if (mqtt.connected()) {
      mqtt.beginPublish(topic.c_str(), topic.length(), false);
      mqtt.print("Unable to update: ");
      mqtt.print(connectable_ble_dev->getAddress().toString().c_str());
      mqtt.print(" Error: ");
      mqtt.print(err_state.c_str());
      mqtt.endPublish();  // does nothing?
    }    
  }

  
#endif//WiFi_AGENT


class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks 
{
    void onResult(BLEAdvertisedDevice *advertisedDevice) {

#ifdef WiFi_AGENT      
      if(try_to_connect){
        if(advertisedDevice->haveServiceUUID() && strcmp(advertisedDevice->getAddress().toString().c_str(), requested_ble_dev) == 0 ){
          pBLEScan -> stop();
          Serial.print("Found connectable device! Name: ");
          Serial.println(logger.getName(advertisedDevice->getAddress().toString()).c_str());
          connectable_ble_dev = advertisedDevice;
          is_connectable = true;
          is_scanning = false;
          return;
        }
      }
#endif//WiFi_AGENT      
      if(advertisedDevice->haveManufacturerData()){
        std::string manufacturer_data = strToHex(advertisedDevice->getManufacturerData()); 
        std::string address = advertisedDevice->getAddress().toString();
        
        if(logger.hasChanged(address, manufacturer_data)){
          std::string topic;
          int rssi = advertisedDevice->getRSSI();
          //type does not get used 
          //if use is wanted, uncomment in the create_ssv_ssb_json_object function
          std::string type = "";
          std::string ts = "null";
          
          if( advertisedDevice->haveName() ){
            logger.addName( address, advertisedDevice->getName() );
          }   
          std::string name = logger.getName(address);
          std::string json_object = create_ssv_ssb_json_object(ble_mac, address,name, rssi, ts , type, manufacturer_data);
               
          Serial.println(json_object.c_str());
          Serial.println("\n");

#ifdef WiFi_AGENT                  
          int len = json_object.length();
          topic.append( MQTT_PREFIX_TOPIC);
          topic.append(address);   
          //now publish the advertisement
          if (mqtt.connected()) {
            mqtt.beginPublish(topic.c_str(), len, false);
            mqtt.print(json_object.c_str());
            mqtt.endPublish();  //does nothing?
          }    
#endif//WiFi_AGENT        
        }
      }
   }
};


static void scanCompleteCB(BLEScanResults scanResults) 
{
  is_scanning = false;
} //scanCompleteCB


void setupBLEScan()
{
  Serial.println("Starting BLE scan...");
  NimBLEDevice::init("");
  //Optional: set the transmit power, default is 3db
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  ble_mac = NimBLEDevice::getAddress().toString();
  Serial.print("BLE MAC: "); Serial.println(ble_mac.c_str());
  pBLEScan = NimBLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(), KEEP_DUPLICATES);  
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  //less or equal to setInterval value
}


void startBLEScan()
{
  //forget about the devices seen in the last BLE_SCAN_TIME interval
  pBLEScan->start(BLE_SCAN_TIME, scanCompleteCB, false);
  is_scanning = true;
}





void setup() 
{
  Serial.begin(115200);
  pinMode(restoreButton, INPUT);

  //setup BLE Hardwareand get BLE MAC
  setupBLEScan();
  
  //Get the last 6 chars from the BLE MAC
  String lastCharsBLEMac = String(ble_mac.c_str());
  lastCharsBLEMac.replace(":","");
  lastCharsBLEMac.toLowerCase();
  //remove chars from index 0 to 5
  lastCharsBLEMac.remove(0, 6);

#ifdef WiFi_AGENT  
  Preferences preferences;
  //open up a storage space under the namespace "collector_cfg" in r/w mode
  preferences.begin("collector_cfg",false);
  //get the ssid if no ssid is stored return the default value of ""
  wlan_ssid = preferences.getString("ssid", "");
  wlan_pass =  preferences.getString("password", "");
  mqtt_server_ip = preferences.getString("broker_ip", "");
  auto_config = preferences.getBool("auto_config", true);
  noConnection = preferences.getBool("noConnection", false );

  //no config data is stored OR restoreButton pressed OR noConnection --> enter wifi ap mode
  if( (wlan_ssid == "" && wlan_pass == "") || digitalRead(restoreButton) == LOW || noConnection == true){
     
     if(digitalRead(restoreButton) == LOW)
        Serial.println("Restore button was pressed!");
     else if(noConnection == true)
        Serial.println("The existing WiFi setup didn't work!");
     else   
        Serial.println("No config files found in storage!");
     
     Serial.println("Entering WIFI AP-Mode");
     
     WiFi.mode(WIFI_AP);   
     WiFi.softAP( String(WLAN_AP_SSID + lastCharsBLEMac).c_str()  , WLAN_AP_PASSWORD);
     Serial.print("Local IP is"); Serial.println(WiFi.softAPIP());
     //reset the variables 
     
     //create a long string with all the wifi config which will be sent to the clients
     //string will be sent on connection event
     currentWiFiConfig = wlan_ssid + "/t" + wlan_pass + "/t" + mqtt_server_ip;  
     wlan_ssid = "";
     wlan_pass = "";
     auto_config = true;
     mqtt_server_ip = "";

     SPIFFS.begin(true);

     server.on("/ssv-logo_blue", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/ssv-logo_blue.png", "image/png");
     });

     initWebSocket();     
     DNSServer dnsServer;
     //if DNSServer is started with "*" for domain name, it will reply with
     //provided IP to all DNS request 
     dnsServer.start(AP_DNS_PORT, "*", WiFi.softAPIP());    
     server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);//only when requested from AP
     server.begin(); 
     
     //create an individual config loop
     while(true){
        dnsServer.processNextRequest();
        ws.cleanupClients();
        
        //get the config data and store it in preferences!
        if(wlan_ssid != ""){
          Serial.println(wlan_ssid);
          Serial.println(wlan_pass);
          
          preferences.putString("ssid", wlan_ssid);
          preferences.putString("password", wlan_pass);
          //try the new config with the next bootup
          preferences.putBool("noConnection", false);
          if(auto_config == false){
            preferences.putBool("auto_config", auto_config);
            preferences.putString("broker_ip", mqtt_server_ip);          
          }
          preferences.end();
          String str = "success!";
          notifyClients(str); 
          delay(50);
          ESP.restart();
        }
     }       
  }else{
    //connect to stored network
    Serial.println("Found config!");
    Serial.print("WiFi SSID: ");  Serial.println(wlan_ssid);
    Serial.print("WiFi Pass: ");   Serial.println(wlan_pass);
    Serial.print("MQTT Broker IP: ");  Serial.println(mqtt_server_ip); 
    Serial.print("AUTO_CONFIG: ");Serial.println(auto_config); 
  }
  
  //start WiFi Connection with received Credentials
  WiFi.begin(wlan_ssid.c_str(), wlan_pass.c_str());

  int timeout = 0;
  while(WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    timeout++;
    if(timeout > 20)
    {
      Serial.println("");
      Serial.println("No WiFi connection! Restarting!");
      //no Connection possible go back to AP Mode after reboot
      preferences.putBool("noConnection", true);
      //close the preferences
      preferences.end();
      ESP.restart();
    }
  }
  Serial.print("Connected to WiFi! Local IP Address is: ");
  Serial.println(WiFi.localIP());

  if(auto_config == false ){
    MQTT_SERVER = mqtt_server_ip;
  }else{
    mDNShostname += lastCharsBLEMac;
    Serial.print("Registering mDNS hostname: "); Serial.println(mDNShostname);
    
    if (!MDNS.begin(mDNShostname.c_str())) {
          Serial.println("Error setting up mDNS responder!");
          ESP.restart();
      } 
    Serial.print("To access, using "); Serial.print(mDNShostname); Serial.println(".local"); 
    
    timeout = 0;
    while (MQTT_SERVER == ""){
      if(timeout >= 10){
        Serial.println("Unable to find mDNS Service! Rebooting..");
        //no Connection possible, go back to AP Mode after reboot
        preferences.putBool("noConnection", true);
        //close the preferences
        preferences.end();
        ESP.restart();
      }
      Serial.print("timeout is: ");Serial.println(timeout);
      MQTT_SERVER = browseService(serviceName, "tcp");
      timeout += 1;
    }
  }
 
  mqtt.setServer(MQTT_SERVER.c_str(), MQTT_PORT);
  mqtt.setCallback(mqtt_receive_callback); 
  
  timeout = 0;
  while(check_mqtt() == false){
    if(timeout >= 10){
      //no Connection possible go back to AP Mode after reboot
      preferences.putBool("noConnection", true);
      preferences.end();
      Serial.println("Unable to connect to MQTT Broker!");
      ESP.restart();
    }
    timeout += 1;  
  }

  //when connected to WiFi MQTT Broker, safe the Broker IP and use it forever
  preferences.putBool("auto_config", false);
  preferences.putString("broker_ip", MQTT_SERVER);
  //sotre this config as succesfull      
  preferences.putBool("noConnection", false);
  //close the preferences
  preferences.end();

#endif//WiFi_AGENT   
  Serial.println("Setup succesfull!");
}


void loop() {

#ifdef WiFi_AGENT  
  bool wifi_good;
  bool mqtt_good;

  wifi_good = check_wifi(false);
  mqtt_good = check_mqtt();
  
  if(wifi_good) {
    mqtt.loop();
    ws.cleanupClients();
  }

  if(try_to_connect && is_connectable){
    updateSensorBox( connectable_ble_dev, serviceUUID, characteristicUUID, (unsigned char*)ble_write_data, ble_write_data_len);  
    //stop searching for connections
    is_connectable = false;
    try_to_connect = false;
  }
#endif//WiFi_AGENT
 
  //only fire off a new scan if we are not already scanning!
  if (not is_scanning) { 
    startBLEScan();
  }
  delay(1);
}
