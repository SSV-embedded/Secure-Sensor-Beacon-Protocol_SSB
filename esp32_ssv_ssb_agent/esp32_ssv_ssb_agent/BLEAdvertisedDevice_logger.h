#ifndef BLEADVERTISEDDEVICE_LOGGER
#define BLEADVERTISEDDEVICE_LOGGER

#include "Arduino.h"
#include <string>

static const int BLE_MAC_SIZE = 18;
//max payload is 29 bytes + 1 byte 0x00
static const int BLE_PAYLOAD_SIZE = 30;
static const int BLE_NAME_SIZE = 30;

struct device {
    char mac[BLE_MAC_SIZE];
    char payload[BLE_PAYLOAD_SIZE];
    size_t last_seen;
    char name[BLE_NAME_SIZE];
};


template <int size>
class BLEAdvertisedDevice_logger{
    public:
        BLEAdvertisedDevice_logger();
        bool hasChanged(const std::string &mac, const std::string &man);
        void addName(const std::string &mac, const std::string &name);
        std::string getName(const std::string &mac);

    private:
        size_t seq;
        struct device devices[size];
        struct device * findDeviceByMac(const std::string &mac);
        struct device * createDevice(const std::string &mac);
        size_t getDeviceAge(struct device * device);  
};


template <int size>
BLEAdvertisedDevice_logger<size>::BLEAdvertisedDevice_logger() : seq(UINT32_MAX) {
    memset(this->devices, 0x00, sizeof(struct device) * size);
}

template <int size>
struct device * BLEAdvertisedDevice_logger<size>::findDeviceByMac(const std::string &mac) {
    for (size_t i = 0; i < size; i++) {
        if (strncmp(this->devices[i].mac, mac.c_str(), BLE_MAC_SIZE) == 0) return &this->devices[i];
    }
    return nullptr;
}

template <int size>
size_t BLEAdvertisedDevice_logger<size>::getDeviceAge(struct device * device) {
    return this->seq - device->last_seen;
}

//create new device in array
//if there is an empty spot in the array, fill it with the new mac
//if there is no empty element in the array, replace the oldest device with the newly received mac-address
template <int size>
struct device * BLEAdvertisedDevice_logger<size>::createDevice(const std::string &mac) { 
    //1. Find empty spot
    for (size_t i = 0 ; i < size; i++) {
        if (strlen(this->devices[i].mac) == 0) {                        
            strncpy(this->devices[i].mac, mac.c_str(), BLE_MAC_SIZE);   
            return &this->devices[i];                                   
        }
    }

    //2. Find oldest spot and delete it
    struct device * cur_device = &this->devices[0];           
    for (size_t i = 1 ; i < size; i++) {
        if (this->getDeviceAge(cur_device) < this->getDeviceAge(&this->devices[i])) { 
            cur_device = &this->devices[i];                                           
        }
    }
    //reset all entries of the old device
    memset(cur_device, 0x00, sizeof(struct device));
    //copy the new mac to the entry of the oldest device        
    strncpy(cur_device->mac, mac.c_str(), BLE_MAC_SIZE); 

    return cur_device;                               
}

template <int size>
bool BLEAdvertisedDevice_logger<size>::hasChanged(const std::string &mac, const std::string &man) {
    //Lookup MAC address
    struct device * candidate = this->findDeviceByMac(mac);

    //Not present -> create new device
    if (candidate == nullptr) {
        candidate = this->createDevice(mac);
    }

    //Mark the device touched
    candidate->last_seen = this->seq++;

    if(strncmp(candidate->payload, man.c_str(), BLE_PAYLOAD_SIZE) == 0) {
        return false;
    }
    strncpy(candidate->payload, man.c_str(), BLE_PAYLOAD_SIZE);
    return true;
}

template <int size>
void BLEAdvertisedDevice_logger<size>::addName(const std::string &mac, const std::string &name){
    //lookup MAC Address
    struct device * candidate = this->findDeviceByMac(mac);

    //Not present -> create new device
    if(candidate == nullptr){
        candidate = this->createDevice(mac);
    }
    //if device doesnt have a name -> copy the received name 
    if( strncmp( candidate->name, name.c_str(), BLE_NAME_SIZE ) != 0 ){
        strncpy(candidate->name, name.c_str(), BLE_NAME_SIZE);
    }
}

template <int size>
std::string BLEAdvertisedDevice_logger<size>::getName(const std::string &mac){
    //lookup MAC Address
    struct device * candidate = this->findDeviceByMac(mac);
    //Not present or no name
    if(candidate == nullptr || strlen(candidate->name) == 0 ){
        return "null";
    }
    std::string str = "";
    str.append(candidate->name);
    return  str;
}

#endif //BLEADVERTISEDDEVICE_LOGGER
