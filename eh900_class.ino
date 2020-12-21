//
//
//
//
// 

#include "eh900_class.h"

namespace{
    //  センサ長の最大値、最小値（setterでのリミットに使用）
    constexpr uint16_t SENSOR_LENGTH_MIN = 6;
    constexpr uint16_t SENSOR_LENGTH_MAX = 24;
    //  液面の上限値[0.1%]
    constexpr uint16_t LIQUID_LEVEL_UPPER_LIMIT = 1000;

    //  タイマー設定の最大値(setterでのリミットに使用)
    constexpr uint16_t TIMER_PERIOD_MAX = 5400; // [s]

    // FRAM ADDRESS:
    //  （予備）フラグ用の領域(256byte)
    constexpr uint16_t FRAM_FLAG_ADDR = 0x0000;

    //  パラメタを保存する領域(256byte) Meter_parameters構造体をそのまま保存
    constexpr uint16_t FRAM_PARM_ADDR = 0x0100;
}


eh900::eh900(void){

    // eh900::init();

}

boolean eh900::init(void){
    boolean initSucceed = false;
    // init FRAM
    if (fram.begin()) {  // you can stick the new i2c addr in here, e.g. begin(0x51);
        Serial.println("Found I2C FRAM");

        // 設定値をFRAMから読み込む
        nvram_get(FRAM_PARM_ADDR, eh_status);
        Serial.print(" .. Sensor Length: "); Serial.println(eh_status.sensor_length);
        Serial.print(" .. Timer period: "); Serial.println(eh_status.timer_period);
        // eh_status.timer_period  = 40;
        // eh_status.sensor_length = 20;

        //  初回起動フラグをクリアしておく
        fram.write8(FRAM_FLAG_ADDR, 1);

        initSucceed = true;
    } else {
        Serial.println("I2C FRAM not identified ...");

    }
    return initSucceed;
}

boolean eh900::storeParameter(void){

    nvram_put(FRAM_PARM_ADDR, eh_status);
    return true;
}

void eh900::setSensorLength(uint16_t value){
    
    if (value > SENSOR_LENGTH_MAX ){
        value = SENSOR_LENGTH_MAX;
    }
    if (value < SENSOR_LENGTH_MIN){
        value = SENSOR_LENGTH_MIN;
    }

    eh_status.sensor_length = value;
}

void eh900::setTimerPeriod(uint16_t value){
    
    if (value > TIMER_PERIOD_MAX){
        value = TIMER_PERIOD_MAX;
    }

    eh_status.timer_period = value;

}

void eh900::setTimerElasped(uint16_t value){
    // 引数がタイマの設定値より大きければ0に設定する
    if (value > eh_status.timer_period){
        value = 0;
    }
    eh_status.timer_elasped = value;
}

boolean eh900::incTimeElasped(void){
    boolean flag=false;

    //  timerの経過時間をカウントアップ
    eh_status.timer_elasped++;
    //  設定時間経過したら経過時間を0に戻してフラグを立てる
    if (eh_status.timer_elasped >= eh_status.timer_period){
        eh_status.timer_elasped = 0;
        flag = true;
    }
    //  毎秒呼ばれるこを期待して、Tick-Tockフラグをセット
    eh_status.f_tick_tock = true;

    return flag;
}

void eh900::setLiquidLevel(uint16_t value){
    // 上限は100

    if (value > LIQUID_LEVEL_UPPER_LIMIT){
        value = LIQUID_LEVEL_UPPER_LIMIT;
    }
    eh_status.liqud_level = value;
}

boolean eh900::hasTickTock(void){
    const boolean flag = eh_status.f_tick_tock;
    eh_status.f_tick_tock = false;
    return flag;

}

//  FRAMに変数の値を書き込む
template <typename T>
const void eh900::nvram_put(uint16_t idx, const T& t){
    const uint8_t* ptr = (const uint8_t *) &t;
    for (int i=0; i < sizeof(t); i++){
        fram.write8(idx + i, *(ptr + i));
    }
}

//  FRAMから変数に値を読み込む
template <typename T>
void eh900::nvram_get(uint16_t idx, T& t){
    uint8_t* ptr = (uint8_t *) &t;
    for (int i=0; i < sizeof(t); i++){
        *(ptr + i) = fram.read8(idx + i);
    }
}
