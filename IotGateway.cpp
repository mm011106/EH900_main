/**************************************************************************/
/*!
    @file     IotGateway.cpp
    @author   Masa

        UART drive + JSON data former

        @section  HISTORY

*/
/**************************************************************************/
#include "IotGateway.h"


/*!
    @brief  payloadに直接JSONデータを書き込む（オーバーライト）
    @param json 大外の { } なしの形のJSONデータ
    @return payloadの中身（文字列）

*/
void IotGateway::setPayload(String json){
  payload = json;

}

/*!
    @brief  payloadに引数のデータをノードして加える  
    @param void
    @return payloadの中身（文字列）

*/
String IotGateway::getPayload(void){
  return("{" + payload + "}");
}

/*!
    @brief  payloadに引数のデータをノードして加える  
    @param key JSONのキー
    @param value 値（文字列）

*/
void IotGateway::addPayload(String key, String value){
  String quote = "\"";
  String node = quote + key + quote + ":" + quote + value + quote;

  IotGateway::joinToPayload(node);

  return;

}
/*!
    @brief  payloadに引数のデータをノードして加える 
    @param key JSONのキー
    @param value 数値（整数）

*/
void IotGateway::addPayload(String key, int32_t value){
  String quote = "\"";
  String node = quote + key + quote + ":" +  String(value) ;

  IotGateway::joinToPayload(node);

  return;
}

/*!
    @brief  payloadに引数のデータをノードして加える  
    @param key JSONのキー
    @param value 数値（小数）
    @param deciPlac 出力する小数点以下桁数

*/
void IotGateway::addPayload(String key, float value, u_int8_t deciPlac){
  String quote = "\"";
  String node = quote + key + quote + ":"  + String(value, deciPlac) ;

  IotGateway::joinToPayload(node);

  return;
}

/*!
    @brief  データセパレータ(,)を入れながらpayloadにノードを足す   (private)
    @param node JSONの情報単位
*/
void IotGateway::joinToPayload(String node){
  if (payload.length() != 0)
    // payloadに何か既に入っている場合
    {payload = payload + "," + node ;} 
  else
    // payloadに何もない場合
    {payload = node;}
  
  return;
}