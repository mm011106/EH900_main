#ifndef _IOTGATEWAY_H_
#define _IOTGATEWAY_H_

#include "HardwareSerial.h"

class IotGateway : public HardwareSerial{

  public:
    /*!
    @brief  コンストラクタ unit32タイプのピン指定だけを受け付けます
    */
    IotGateway(uint32_t pin_rx, uint32_t pin_tx) : HardwareSerial(pin_rx, pin_tx){

    };

    void setPayload(String json);
    String getPayload(void);

    void addPayload(String key, String value);
    void addPayload(String key, int32_t value);
    void addPayload(String key, float value, u_int8_t deciPlac);
    
    
    /*!
    @brief  payloadをクリア
    @param void
    @return void
    */
    void clearPayload(void){
      payload="";
    };
    
    /*!
    @brief  payloadをUARTに出力
    @param void
    @return void
    */
    void sendPayload(void){
      HardwareSerial::println(getPayload());
    };

  private:
    String payload;
    void joinToPayload(String node);
};

#endif // _IOTGATEWAY_H_