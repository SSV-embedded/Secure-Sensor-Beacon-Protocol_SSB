#include "utilities.h"

std::string strToHex(const std::string& input)
{
    static const char hex_digits[] = "0123456789ABCDEF";
    std::string result;
    result.reserve(input.length() * 2);
    for (unsigned char c : input) {
      result.push_back(hex_digits[c >> 4]);
      result.push_back(hex_digits[c & 15]);
    }
    return result;
}


/*
 * Given a byte array of length (n), return the ASCII hex representation
 * and properly zero pad values less than 0x10.
 * String(0x08, HEX) will yield '8' instead of the expected '08' string
 */
String hexToStr(uint8_t* arr, int n)
{
  String result;
  result.reserve(2*n);
  for (int i = 0; i < n; ++i) {
    if (arr[i] < 0x10) {result += '0';}
    result += String(arr[i], HEX);
  }
  return result;
}


std::string strToBinary(const char * string, int len)
{
  std::string result = "";
  //string has to have an even length to fill a whole byte
  if(len %2 != 0){
    return "";
  }
  
  for(int i = 0; i < len ;i += 2){
    unsigned char arr[2];
    for(int j = 0; j < 2; j++){
      if(string[i+j] >= '0' && string[i+j] <= '9'){
        arr[j] = (string[i+j] - '0');
      }else if(string[i+j] >= 'A' && string[i+j] <= 'F'){
        arr[j] = (string[i+j] - 'A' + 10);
      }else if(string[i+j] >= 'a' && string[i+j] <= 'f'){
        arr[j] = (string[i+j] - 'a' + 10);
      }else{ 
        return ""; //the string contained a sign outside of the defined area
      }
    }
    result += (char) ( (arr[0]<<4) | (arr[1]&0xF) ); 
  }
  return result;
}


std::string create_ssv_ssb_json_object ( std::string gw, std::string address, std::string name, int rssi, std::string ts, std::string type, std::string manufacturer_data ){

  std::string json_object = "{\"gw\":\"" ;
  json_object.append( gw );
  json_object.append( "\",\"address\":\"" );
  json_object.append( address) ;
  json_object.append( "\",\"name\":\"" );
  json_object.append( name );
  
  json_object.append( "\",\"rssi\":" );
  json_object.append( String(rssi).c_str() );
  json_object.append( ",\"ts\":" );
  json_object.append( ts );
  //json_object.append( ",\"type\":\"" );
  //json_object.append( type );
  //json_object.append( "\"" );
  json_object.append( ",\"data\":\"" );
  json_object.append( manufacturer_data );
  json_object.append( "\"}" );
  
  return json_object;
}
