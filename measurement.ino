#include "measurement.h"


namespace{  //  I2C adress 
    constexpr uint16_t I2C_ADDR_ADC            = 0x48;
    constexpr uint16_t I2C_ADDR_CURRENT_ADJ    = 0x60;
    constexpr uint16_t I2C_ADDR_V_MON          = 0x49;
    constexpr uint16_t I2C_ADDR_PIO            = 0x20;
}

// ADの読み値から電圧値を計算するための系数 [/ micro Volts/LSB]
// 3.3V電源、差動計測（バイポーラ出力）を想定
namespace{
    constexpr float ADC_READOUT_VOLTAGE_COEFF_GAIN_TWOTHIRDS    = 187.506;  //  FS 6.144V * 1E6/32767
    constexpr float ADC_READOUT_VOLTAGE_COEFF_GAIN_ONE          = 125.004;  //  FS 4.096V * 1E6/32767
    constexpr float ADC_READOUT_VOLTAGE_COEFF_GAIN_TWO          = 62.5019;  //  FS 2.048V * 1E6/32767
    constexpr float ADC_READOUT_VOLTAGE_COEFF_GAIN_FOUR         = 31.2509;  //  FS 1.024V * 1E6/32767
    constexpr float ADC_READOUT_VOLTAGE_COEFF_GAIN_EIGHT        = 15.6255;  //  FS 0.512V * 1E6/32767
    constexpr float ADC_READOUT_VOLTAGE_COEFF_GAIN_SIXTEEN      = 7.81274;  //  FS 0.256V * 1E6/32767
}

//  PIO関連 定数
namespace{
    //  PIOのポート番号の設定と論理レベル設定
    constexpr uint16_t PIO_CURRENT_ENABLE    = 4 ;  // ON = LOW,  OFF = HIGH
    constexpr uint16_t PIO_CURRENT_ERRFLAG   = 0 ;  // LOW = FAULT(short or open), HIGH = NOMAL
    // 電流源コントロールロジック定義
    constexpr uint16_t CURRENT_OFF = HIGH ;
    constexpr uint16_t CURRENT_ON = LOW ;
}

//  計測に使う定数
namespace{
    //  センサの単位長あたりのインピーダンス[ohm/inch]
    constexpr float SENSOR_UNIT_IMP = 11.6;

    // 電流計測時の電流電圧変換係数(R=5kohm, Coeff_Isenosr=0.004(0.2/50ohm)の時) [/ V/A]
    constexpr float CURRENT_MEASURE_COEFF = 20.000;

    //  電圧計測のアッテネータ系数  1/10 x 2/5 の逆数  実際の抵抗値での計算
    constexpr float ATTENUATOR_COEFF = 24.6642;

    //  センサの熱伝導速度  測定待ち時間の計算に必要
    constexpr float HEAT_PROPERGATION_VEROCITY = 7.9; // inch/s

    // AD変換時の平均化回数 １回測るのに10msかかるので注意  10回で100ms
    constexpr uint16_t ADC_AVERAGE_DEFAULT = 10;

    //  電流源設定用DAC MCP4725 1Vあたりの電流[0.1mA]  A/V
    constexpr uint16_t  CURRENT_SORCE_VI_COEFF  = 56;

    //  DAC MCP4275の1Vあたりのカウント (3.3V電源にて） COUNT/V
    constexpr uint16_t DAC_COUNT_PER_VOLT = 1241;

    //  DAC80501 1Vあたりのカウント(2.5VFS時）  COUNT/V
    constexpr uint16_t VMON_COUNT_PER_VOLT = 26214;
}

// Measurement::Measurement(eh900* pModel) : LevelMeter(pModel) {}
/*!
 * @brief 計測モジュールの初期化
 * @returns True：全てのデバイスが初期化された False：初期化できないデバイスがあった
 */
boolean Measurement::init(void){

    if (!LevelMeter){
        Serial.println("Measurement::init Parameter is null.");
        return false;
    }

    boolean f_init_succeed = true;
    boolean status = false;

    Serial.print("Meas init -- ");

    // //  必要なパラメタの設定 LevelMeterから読み込む
    Measurement::renew_sensor_parameter();

    // //      センサの抵抗値
    // sensor_resistance = 11.6 * (float)LevelMeter->getSensorLength();
    // //      センサ長に応じた計測待ち時間[ms]を設定   マージンとして1.2倍
    // delay_time = LevelMeter->getSensorLength() * (uint16_t)(1/HEAT_PROPERGATION_VEROCITY * 1000.0 * 1.2);
    
    // Serial.print("Sensor Length:"); Serial.println(LevelMeter->getSensorLength());
    // Serial.print("Delay Time:"); Serial.println(delay_time);
    // Serial.print("Sensor R:"); Serial.println(sensor_resistance);

    Serial.print("AD Error Comp 01: "); Serial.println(LevelMeter->getAdcErrComp01()*100);
    Serial.print("AD Error Comp 23: "); Serial.println(LevelMeter->getAdcErrComp23()*100);
    Serial.print("AD OFFSET Comp 01: "); Serial.println(LevelMeter->getAdcOfsComp01());
    Serial.print("AD OFFSET Comp 23: "); Serial.println(LevelMeter->getAdcOfsComp23());

    Serial.print("Current Sorce setting: "); Serial.println(LevelMeter->getCurrentSetting());
    Serial.print("Vmon Offset [LSB]: "); Serial.println(LevelMeter->getVmonOffset());

    // 電流源設定用DAC  初期化
    if(current_adj_dac){
        delete current_adj_dac;
    }

    current_adj_dac = new Adafruit_MCP4725;

    status = current_adj_dac->begin(I2C_ADDR_CURRENT_ADJ, &Wire);
    if (!status) { 
        Serial.println("error on Current Source DAC.  ");
        f_init_succeed = false;
    } else {
        // 電流値設定
        Measurement::setCurrent(LevelMeter->getCurrentSetting());
    }

    // アナログモニタ用DAC  初期化
    if(v_mon_dac){
        delete v_mon_dac;
    }

    v_mon_dac = new DAC80501;

    status = v_mon_dac->begin(I2C_ADDR_V_MON, &Wire);
    if (!status) { 
        Serial.println("error on Analog Monitor DAC.  ");
        f_init_succeed = false;
    } else {
        status = v_mon_dac->init();
         if (!status) { 
            Serial.println("error on Analog Monitor DAC.  ");
            f_init_succeed = false;
        } else {
            // アナログモニタ出力   リセット 
            Measurement::setVmon(0);
        }
    }

    //  PIOポート設定
    if(pio){
        delete pio;
    }

    pio = new Adafruit_MCP23008;

    status = pio->begin(I2C_ADDR_PIO, &Wire); 
    if (!status) { 
        Serial.println("error on PIO.  ");
        f_init_succeed = false;
    } else {
        //  set IO port 
        pio->pinMode(PIO_CURRENT_ERRFLAG, INPUT);
        pio->pullUp(PIO_CURRENT_ERRFLAG, HIGH);  // turn on a 100K pullup internally

        pio->pinMode(PIO_CURRENT_ENABLE, OUTPUT);
        pio->digitalWrite(PIO_CURRENT_ENABLE, CURRENT_OFF);
    }

    //  ADコンバータ設定    PGA=x2   2.048V FS
    if(adconverter){
        delete adconverter;
    }

    adconverter = new Adafruit_ADS1115(I2C_ADDR_ADC);
    adconverter->begin();
    adconverter->setGain(GAIN_TWO); 

    Serial.print("DA-current:"); Serial.print((uint32_t)current_adj_dac,HEX); Serial.print("/");Serial.println(sizeof(*current_adj_dac));
    Serial.print("DA-Vmon:"); Serial.print((uint32_t)v_mon_dac,HEX); Serial.print("/");Serial.println(sizeof(*v_mon_dac));
    Serial.print("PIO:"); Serial.print((uint32_t)pio,HEX); Serial.print("/");Serial.println(sizeof(*pio));
    Serial.print("ADC:"); Serial.print((uint32_t)adconverter,HEX); Serial.print("/");Serial.println(sizeof(*adconverter));
    Serial.print("eh900:"); Serial.print((uint32_t)LevelMeter,HEX); Serial.print("/");Serial.println(sizeof(*LevelMeter));

    Serial.println("Measurement::init  Fin. --"); 

    return f_init_succeed;

}

/*!
 * @brief センサ長の設定に基づき、抵抗値やディレイを設定する
 * 
 */
void Measurement::renew_sensor_parameter(void){

    //      センサの抵抗値
    sensor_resistance = SENSOR_UNIT_IMP * (float)LevelMeter->getSensorLength();

    //      センサ長に応じた計測待ち時間[ms]を設定   マージンとして1.2倍
    delay_time = LevelMeter->getSensorLength() * (uint16_t)(1/HEAT_PROPERGATION_VEROCITY * 1000.0 * 1.2);
    
    Serial.print("Sensor Length:"); Serial.println(LevelMeter->getSensorLength());
    Serial.print("Delay Time:"); Serial.println(delay_time);
    Serial.print("Sensor R:"); Serial.println(sensor_resistance);


}

/*!
 * @brief 電流源をOnにする
 * @returns True 負荷に異常なしでOnできた, False 負荷に異常がありOnできなかった
 */
boolean Measurement::currentOn(void){

    Serial.print("currentCtrl:ON -- "); 
    pio->digitalWrite(PIO_CURRENT_ENABLE, CURRENT_ON);
    delay(10); // エラー判定が可能になるまで10ms待つ
    
    if (pio->digitalRead(PIO_CURRENT_ERRFLAG) == LOW){
        f_sensor_error = true;
        pio->digitalWrite(PIO_CURRENT_ENABLE,CURRENT_OFF);
        Serial.print(" FAIL.  ");
    } else {
        f_sensor_error = false;
        delay(100); // issue1: 電流のステイブルを待つ
        Serial.print(" OK.  ");
    }
    Serial.println("Fin. --");

    return !f_sensor_error;
}

/*!
 * @brief 電流源をOffにする
 */
void Measurement::currentOff(void){
    Serial.print("currentCtrl:OFF  -- ");
    pio->digitalWrite(PIO_CURRENT_ENABLE, CURRENT_OFF);      
    Serial.println(" Fin. --");
}

/*!
 * @brief 電流源の電流値を設定する
 * @param current current in [0.1milliAmp]
 *          設定値は67mA〜83mAの範囲：デフォルト75mA
 * 
 */
void Measurement::setCurrent(uint16_t current){  // current in [0.1milliAmp]

    Serial.print("currentSet: -- "); 
    if ( 670 < current && current < 830){
        uint16_t value = (( current - 666 ) * DAC_COUNT_PER_VOLT) / CURRENT_SORCE_VI_COEFF;
        // current -> vref converting function
        current_adj_dac->setVoltage(value, false);
        Serial.print(" DAC changed. " ); 
      }
    Serial.println("Fin. --" ); 
}

/*!
 * @brief 電流源のステータスを返す
 * @returns True: 電流Onの設定で正常に電流を供給している, False:電流がoff もしくは 負荷異常
 */
boolean Measurement::getStatus(void){
    return (   (pio->digitalRead(PIO_CURRENT_ENABLE)==CURRENT_ON)    \
            && (pio->digitalRead(PIO_CURRENT_ERRFLAG) == HIGH)        \
    );

}
/*!
 * @brief 液面計測を1回行う.
 *          熱伝導速度も考慮して時間待ちする.
 * @returns True:正常に終了, False:電流源が動作せず計測不可
 */
boolean Measurement::measSingle(void){
    
    // boolean flag = false;
    Serial.println("singleShot: -- "); 
    

    if (!Measurement::currentOn()){
        Measurement::currentOff();
    } else {
        Serial.print("meas start..  ");

        //  センサへの熱伝導待ち時間の間に3回計測する（動いていますというフィードバックのため）
        for (uint16_t i =0 ; i < 3; i++){
            Measurement::readLevel();
            delay(delay_time/3);
        }
        //  確定値の計測
        Measurement::readLevel();
        Measurement::currentOff();

        Serial.println("single: meas end.");
    }
    
    Serial.println("singleShot: Fin. --"); 

    return !f_sensor_error;
}

/*!
 * @brief 液面を計測し結果を保存
 * 電流のon/offは感知しない 
 */
void Measurement::readLevel(void){
    // uint32_t vout = Measurement::read_voltage(); // [micro Volt]
    uint32_t iout = Measurement::read_current(); // [micro Amp]

    // calc L-He level from the mesurement
    float ratio =1.0;
    // 電流が計測されていない場合[1mA以下]   0%  にする。
    if (iout != 0){
        ratio = ((float)Measurement::read_voltage()/(float)iout) / sensor_resistance ;
        Serial.print(" Resistance = "); Serial.println( ratio * sensor_resistance);
        Serial.print(" Ratio = "); Serial.println( ratio );
    } else { 
        Serial.print(" No current flow! ");
    }
    // センサの抵抗値誤差のマージンとして　2%　少な目に表示する
    uint16_t result = round(( 1.0 - ratio*1.02) * 1000);  // [0.1%]
    Serial.print(" Level = "); Serial.println( result);
    LevelMeter->setLiquidLevel(result);

}
/*!
 * @brief センサの電圧を計測する
 * @returns 計測した電圧    [microVolt]
 */
uint32_t Measurement::read_voltage(void){  

    float results = 0.0;
    float readout = 0.0;
    uint16_t   avg = ADC_AVERAGE_DEFAULT;     // averaging

    Serial.print("Voltage Meas: read_voltage(0-1): ");

    for (uint16_t i = 0; i < avg; i++){
    //   results += (float)adconverter.readADC_Differential_0_1();
        readout = (float)(adconverter->readADC_Differential_0_1() - LevelMeter->getAdcOfsComp01());
        Serial.print(", "); Serial.print(readout);  
        results += readout;
    }

//    uint16_t gain_setting = ads.getGain();
    float adc_gain_coeff=0.0;
    switch (adconverter->getGain()){
        case GAIN_TWOTHIRDS:
            adc_gain_coeff = ADC_READOUT_VOLTAGE_COEFF_GAIN_TWOTHIRDS;
        break;

        case GAIN_ONE:
            adc_gain_coeff = ADC_READOUT_VOLTAGE_COEFF_GAIN_ONE;
        break;

        default:
            adc_gain_coeff = ADC_READOUT_VOLTAGE_COEFF_GAIN_TWO;
        break;
    }

    results = results / (float)avg * adc_gain_coeff * LevelMeter->getAdcErrComp01() * ATTENUATOR_COEFF;

// Lower limmit 
    // if (results < 1.0){
    //     results = 1.0;
    // }

    Serial.print(" "); Serial.print(results); Serial.println(" uV: Fin. --");

    return round(results);
}

/*!
 * @brief センサに流れている電流を計測
 * @returns 計測した電流    [microAmp]
 */
uint32_t Measurement::read_current(void){  // return measured current in [microAmp]

    float results = 0.0;
    float readout = 0.0;
    uint16_t avg = ADC_AVERAGE_DEFAULT;  // averaging

    Serial.print("Current Meas: read_voltage(2-3): ");

    for (uint16_t i = 0; i < avg; i++){
        readout = (float)(adconverter->readADC_Differential_2_3() - LevelMeter->getAdcOfsComp23());
        Serial.print(", "); Serial.print(readout);  
        results += readout;
    }

    float adc_gain_coeff=0.0;
    switch (adconverter->getGain()){
      case GAIN_TWOTHIRDS:
        adc_gain_coeff = ADC_READOUT_VOLTAGE_COEFF_GAIN_TWOTHIRDS;
        break;

      case GAIN_ONE:
        adc_gain_coeff = ADC_READOUT_VOLTAGE_COEFF_GAIN_ONE;
        break;

      default:
        adc_gain_coeff = ADC_READOUT_VOLTAGE_COEFF_GAIN_TWO;
        break;
    }

    Serial.print("reading conveted to Current Out(2-3):");
    // results = results / (float)avg * coeff * ADC_ERR_COMPENSATION; // reading in microVolt
    results = results / (float)avg * adc_gain_coeff * LevelMeter->getAdcErrComp23(); // reading in microVolt
    results = results / (float)CURRENT_MEASURE_COEFF; // convert voltage to current.
    Serial.print(results);
    Serial.println(" uA: Fin. --");
    
  return round(results);
}
/*!
 * @brief アナログモニタ出力の電圧を設定する(100%=1.1V, 0%=0.1V)
 * @param value 液面 [0.1%]    上限：100.0%
 */
void Measurement::setVmon(uint16_t value){
    uint16_t da_value=0;

    // debug
    // return;

    //  100.0%以下の値ならそのまま設定、それ以外は更新しない
    if (value <= 1000) {
        // da_value = ( VMON_COUNT_PER_VOLT * (value + 100) ) / 1000;
        da_value = (( VMON_COUNT_PER_VOLT * value ) / 1000) + (uint16_t)((VMON_COUNT_PER_VOLT / 10) - LevelMeter->getVmonOffset());

    //     100.0% = 1.1V, 0%=0.1V 
        v_mon_dac->setVoltage(da_value);
    }

}
