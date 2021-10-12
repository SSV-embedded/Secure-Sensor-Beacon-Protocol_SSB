/*
   esp32_ssv_ssb.ino example source code using SSV/SSB protocol for ESP32 on Arduino platform

   Written in 2020 by Jan Szücs <jsz@ssv-embedded.de>

   To the extent possible under law, the author(s) have dedicated all copyright
   and related and neighboring rights to this software to the public domain
   worldwide. This software is distributed without any warranty.

   You should have received a copy of the CC0 Public Domain Dedication along with
   this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

#include "fragments.h"

#include<BLEDevice.h>               //libraries neccesary to run BLE Advertising
#include<BLEUtils.h>
#include<BLEServer.h>
#include <Wire.h>                   //libraries for the sensor
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
//#include <EEPROM.h>

typedef unsigned char       u8_t;
typedef unsigned short      u16_t;
typedef unsigned int        u32_t;

// Sensor Data
#define S_DATA_SIZE_LEN 1
#define S_DATA_SIZE_TYPE 1
#define S_DATA_SIZE_HEADER (S_DATA_SIZE_LEN+S_DATA_SIZE_TYPE)
// #define SENSOR_ID_MAGNET_DEF 0x80
// #define SENSOR_AMOUNT_CHAN_MAGNET 3 // Sensor ID modified for each channel
// #define SENSOR_MAX_SIZE_DATA 255 // len is only 1 Byte
#define SENSOR_ID_TEMP_DEF 0x84
#define SENSOR_ID_PRES_DEF 0x88
#define SENSOR_ID_HUM_DEF 0x8C

#define uS_TO_S_FACTOR 1000000ULL   // Conversion factor for micro seconds to seconds 
#define delay_time 800              // Time needed to send the same fragments multiple times to secure no package loss
#define EEPROM_SIZE  257

typedef struct{
  u8_t len;
  u8_t type;
  void *data;
} sensor_data;

// Globals
static u8_t payload_buf[FRAG_SIZE_PAYLOADBUF];
static sensor_data bme280_data[3];
static size_t len_payload;
static frag_t *cur_frag;
// Time each fragment is advertised.
static u32_t intv_br = 2000; // 2s
// The Intervall between each message send
static u32_t intv_sleep = 10; // 10s
// The Intervall between fragment change
static u32_t intv_hotfix = 125; // 125ms

// Variable stored in the rtc memory, to save its value during deep sleep
RTC_DATA_ATTR uint32_t sequenz_deep_sleep_save;
// Variables used to store sequence data in flash
//uint8_t flash_storage_addres;
//uint16_t flash_storage_value;

// ble and sensor objects
BLEAdvertising *pAdvertising;
Adafruit_BME280 bme;

// Functions

void get_bme280_val(sensor_data *s_data){

  static float ftemp, fpress, fhumid;

  ftemp  = bme.readTemperature();
  fpress = bme.readPressure() / 1000 ;  // division necessary to report pressure in hPa in Node-Red
  fhumid = bme.readHumidity();

  s_data[0].data = &ftemp;
  s_data[0].len = (u8_t) sizeof(float);
  s_data[0].type = SENSOR_ID_TEMP_DEF;
  s_data[1].data = &fpress;
  s_data[1].len = (u8_t) sizeof(float);
  s_data[1].type = SENSOR_ID_PRES_DEF;
  s_data[2].data = &fhumid;
  s_data[2].len = (u8_t) sizeof(float);
  s_data[2].type = SENSOR_ID_HUM_DEF;
}

int make_payload(sensor_data *s_data, size_t len_s_data, u8_t *payload_buf, size_t *len_payload){

  int errcode = 0;
  *len_payload = 0;
  int i = 0;
  
  for(;i<len_s_data; i++){
    *len_payload += (s_data[i].len + S_DATA_SIZE_HEADER);
  }
  
  if(*len_payload > FRAG_SIZE_PAYLOADBUF){
    errcode = -1;
    // ERROR: Payload to long!
    return errcode;
  }
  
  i = 0;
  *len_payload = 0;
  
  for(;i<len_s_data; i++){
    *(payload_buf+(*len_payload)) = s_data[i].len;
    *((payload_buf+(*len_payload)+S_DATA_SIZE_LEN)) = s_data[i].type;
    memcpy(payload_buf+(*len_payload)+S_DATA_SIZE_HEADER, s_data[i].data, s_data[i].len);
    *len_payload += (s_data[i].len + S_DATA_SIZE_HEADER);
  }
  
  return errcode;
}

/*
void backup_sequence(uint32_t sequence){
 
  if(sequence % 2048 == 0){
   flash_storage_value++;
       
   if(flash_storage_value == 0x100){
    flash_storage_value = 0;     
    EEPROM.write(0,flash_storage_addres + 1 );    
    EEPROM.commit();   
   }
   EEPROM.write(flash_storage_addres, flash_storage_value);
   EEPROM.commit(); 
  }
}
*/

void setup() {

/*   
   EEPROM.begin(EEPROM_SIZE);
   
   if(EEPROM.read(0) == 255 && EEPROM.read( 1 ) == 255 ){
     EEPROM.write(0,0);
     EEPROM.commit();
     
     EEPROM.write(1,0);  
     EEPROM.commit();
   }

   flash_storage_addres = EEPROM.read(0) + 1;  
   flash_storage_value  = EEPROM.read(flash_storage_addres);    
    
   uint32_t flash_counter_value = ( (flash_storage_addres-1)*256 + flash_storage_value)*2048;
    
   // After the deep sleep the setup() is executed, only copy the sequence from flash, if the current value of the
   // Deep Sleep sequence variable is lower -> newly initialized
   if(sequenz_deep_sleep_save < flash_counter_value){
     sequenz_deep_sleep_save = flash_counter_value;
   }
*/     
   // Start up the Sensor (here BME280)
   bme.begin();
           
   // Set up the BLE server for advertising
   BLEDevice::init("");
    
   BLEServer *pServer = BLEDevice::createServer();
  
   pAdvertising = pServer->getAdvertising();
     
   // Set the local device name as scan response data, not as advertisement data
   BLEAdvertisementData scanResponse;
     
   scanResponse.setName("SFS/ESP32-BME280");   
   pAdvertising->setScanResponseData(scanResponse);
    
}

void loop() {
    
  cur_frag = frag_init(sequenz_deep_sleep_save);
  int err;

  get_bme280_val(bme280_data);
  make_payload(bme280_data, 3, payload_buf, &len_payload);

  err = frag_setpayload(payload_buf, len_payload);
  cur_frag = frag_getfragment();

  while(cur_frag){
    // Create an BLE-API specific struct 'ad' that contains the advertisemnet
    // data. In this case this is the data from the current fragment
    // cur_frag->data. Often the length of the data has to be addded, because
    // cur_frag->data is a pointer. In addtition a header sometimes has to be
    // set. For more information see:
    // https://www.silabs.com/community/wireless/bluetooth/knowledge-base.entry.html/2017/02/10/bluetooth_advertisin-hGsf

    BLEAdvertisementData advert;
    //convert char array to string
    char buffer_array[FRAG_MAX_SIZE_ADV_DATA];
    memcpy(buffer_array, cur_frag->data, cur_frag->len);
    std::string advertisement_str( buffer_array, cur_frag->len );
    //set the data as manufacturer data
    advert.setManufacturerData(advertisement_str);
    //set the flags
    advert.setFlags(0x04);

    pAdvertising->setAdvertisementData(advert);
    pAdvertising->start(); 

    delay(intv_br);

    pAdvertising->stop(); 
    
    cur_frag = frag_getfragment();
    delay(intv_hotfix);
  }
  sequenz_deep_sleep_save++;

//  backup_sequence(sequenz_deep_sleep_save);
  
  //go to deepsleep for previously defined time
  esp_sleep_enable_timer_wakeup(intv_sleep * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}
