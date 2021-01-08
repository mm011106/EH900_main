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

    // FRAM I2C Address
    constexpr uint16_t I2C_ADDR_FRAM = 0x50;
    // FRAM 領域（予備）フラグ用の領域(256byte)
    constexpr uint16_t FRAM_FLAG_ADDR = 0x0000;
    // FRAM 領域    パラメタ保存(256byte) Meter_parameters構造体をそのまま保存
    constexpr uint16_t FRAM_PARM_ADDR = 0x0100;
}


/*!
 *    @brief  FRAMを設定して、液面計の設定パラメタを読み込む
 *    @return True:パラメタ読み込み成功, otherwise false.
 */
boolean eh900::init(void){
    boolean initSucceed = false;
    // init FRAM
    if (fram.begin(I2C_ADDR_FRAM)) {  // you can stick the new i2c addr in here, e.g. begin(0x51);
        Serial.println("Found I2C FRAM");

        // 設定値をFRAMから読み込む
        nvram_get(FRAM_PARM_ADDR, eh_status);
        Serial.print(" .. Sensor Length: "); Serial.println(eh_status.sensor_length);
        Serial.print(" .. Timer period: "); Serial.println(eh_status.timer_period);
        //  初回起動フラグをクリアしておく
        fram.write8(FRAM_FLAG_ADDR, 1);

        initSucceed = true;
    } else {
        Serial.println("I2C FRAM not identified ...");

    }

    Serial.print("FRAM:"); Serial.print((uint32_t)&fram,HEX); Serial.print("/");Serial.println(sizeof(fram));
    Serial.print("eh_status:"); Serial.print((uint32_t)&eh_status,HEX); Serial.print("/");Serial.println(sizeof(eh_status));
    
    return initSucceed;
}

/*!
 *    @brief  液面計の設定パラメタをFRAMに保存
 *    @return つねにTrue
 */
boolean eh900::storeParameter(void){

    nvram_put(FRAM_PARM_ADDR, eh_status);
    return true;
}

/*!
 *    @brief  液面計の設定パラメタをFRAMから読み出し
 *    @return つねにTrue
 */
boolean eh900::recallParameter(void){

    nvram_get(FRAM_PARM_ADDR, eh_status);
    return true;
}

/*!
 *    @brief  センサ長を設定する
 *    @param  value センサ長[inch]：
 *              センサ長はSENSOR_LENGTH_MAX から SENSOR_LENGTH_MIN に制限される
 */
void eh900::setSensorLength(uint16_t value){
    
    if (value > SENSOR_LENGTH_MAX ){
        value = SENSOR_LENGTH_MAX;
    }
    if (value < SENSOR_LENGTH_MIN){
        value = SENSOR_LENGTH_MIN;
    }

    eh_status.sensor_length = value;
}

/*!
 *    @brief  タイマ周期を設定する
 *    @param  value タイマ周期[s]：
 *              valueの上限はTIMER_PERIOD_MAX
 */
void eh900::setTimerPeriod(uint16_t value){
    
    if (value > TIMER_PERIOD_MAX){
        value = TIMER_PERIOD_MAX;
    }

    eh_status.timer_period = value;

}

/*!
 *    @brief  タイマ経過時間をセットする
 *    @param  value 経過時間[s]：
 *              valueの上限はタイマ周期、それ以上なら0に設定される
 */
void eh900::setTimerElasped(uint16_t value){
    // 引数がタイマの設定値より大きければ0に設定する
    if (value > eh_status.timer_period){
        value = 0;
    }
    eh_status.timer_elasped = value;
}

/*!
 *    @brief  タイマの経過時間を１秒進める. 
 *              メンバ変数eh_status.f_tick_tock をtrueにセットする
 *    @return True：タイマ周期に設定された時間になった（タイムアップ）  False:それ以外
 */
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

/*!
 *    @brief  計測した液面を保存する
 *    @param  value 液面[0.1%]:
 *              LIQUID_LEVEL_UPPER_LIMITで制限される
 */
void eh900::setLiquidLevel(uint16_t value){
    // 上限は100

    if (value > LIQUID_LEVEL_UPPER_LIMIT){
        value = LIQUID_LEVEL_UPPER_LIMIT;
    }
    eh_status.liqud_level = value;
}
/*!
 *    @brief  １秒クロックのフラグを確認    この関数を呼ぶと当該フラグはクリアされる
 *    @return True：前回チェック後に１秒クロックが来た  False:まだ来ていない
 */
boolean eh900::hasTickTock(void){
    const boolean flag = eh_status.f_tick_tock;
    eh_status.f_tick_tock = false;
    return flag;

}

/*!
 *    @brief  FRAMに変数の値を書き込む
 *    @param  idx 書き込み開始アドレス
 *    @param  t 書き込むデータ（変数、構造体など）
 */
template <typename T>
void eh900::nvram_put(uint16_t idx, const T& t){
    const uint8_t* ptr = (const uint8_t *) &t;
    for (int i=0; i < sizeof(t); i++){
        fram.write8(idx + i, *(ptr + i));
    }
}

/*!
 *    @brief  FRAMから変数に値を読み込む
 *    @param  idx 読み込み開始アドレス
 *    @param  t 読み込む変数（書き込みの時と型が同じであることが必須）
 */
template <typename T>
void eh900::nvram_get(uint16_t idx, T& t){
    uint8_t* ptr = (uint8_t *) &t;
    for (int i=0; i < sizeof(t); i++){
        *(ptr + i) = fram.read8(idx + i);
    }
}
