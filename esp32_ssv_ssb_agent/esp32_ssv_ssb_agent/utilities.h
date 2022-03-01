#ifndef UTILITIES_H
#define UTILITIES_H

#include "Arduino.h"

std::string strToHex(const std::string& input);

String hexToStr(uint8_t* arr, int n);

std::string strToBinary(const char* string, int len);

std::string create_ssv_ssb_json_object ( std::string gw, 
                                         std::string address, 
                                         std::string name, 
                                         int rssi,
                                         std::string ts,
                                         std::string type,
                                         std::string manufacturer_data );

#endif//UTILITIES_H
