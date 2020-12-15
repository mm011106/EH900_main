#include "measurement.h"

Measurement::Measurement(void ){
}

boolean Measurement::init(eh900* pModel){
    eh900* LevelMeter = pModel;
    boolean initSucceed = true;
    boolean status = false;

    Serial.print("Meas init -- "); 
    //  必要なパラメタの設定 LevelMeterから読み込む
    //      センサの抵抗値
    sensor_resistance = 11.6 * (float)LevelMeter->getSensorLength();
    //      センサ長に応じた計測待ち時間[ms]を設定   マージンとして1.2倍
    delay_time = LevelMeter->getSensorLength() * (uint16_t)(1/HEAT_PROPERGATION_VEROCITY * 1000.0 * 1.2);
    
    // Serial.print("Delay Time:"); Serial.println(delay_time);
    Serial.print("Sensor Length:"); Serial.println(LevelMeter->getSensorLength());
    // Serial.print("Sensor R:"); Serial.println(sensor_resistance);

    //  ADコンバータ 補正係数    
    adc_err_comp_diff_0_1 = LevelMeter->getAdcErrComp01();
    adc_err_comp_diff_2_3 = LevelMeter->getAdcErrComp23();
    adc_OFS_comp_diff_0_1 = LevelMeter->getAdcOfsComp01();
    adc_OFS_comp_diff_2_3 = LevelMeter->getAdcOfsComp23();
    //  電流源設定初期値（調整済みの値）
    current_source_default = LevelMeter->getCurrentSetting();

    Serial.print("AD Error Comp 01: "); Serial.println(adc_err_comp_diff_0_1);
    Serial.print("AD Error Comp 23: "); Serial.println(adc_err_comp_diff_2_3);
    Serial.print("Current Sorce setting: "); Serial.println(current_source_default);


    // 電流源設定用DAC  初期化
    status = current_adj_dac.begin(I2C_ADDR_CURRENT_ADJ, &Wire);
    if (!status) { 
        Serial.print("error on Current Sorce DAC.  ");
        initSucceed = false;
    } else {
        // 電流値設定
        Measurement::setCurrent(current_source_default);
    }

    // アナログモニタ用DAC  初期化
    status = v_mon_dac.begin(I2C_ADDR_V_MON, &Wire);
    if (!status) { 
        Serial.print("error on Analog Monitor DAC.  ");
        initSucceed = false;
    } else {
        // アナログモニタ出力   リセット 
        Measurement::setVmon(0);
    }

    //  PIOポート設定
    status = pio.begin();      // use default address 0x20
    if (!status) { 
        Serial.print("error on PIO.  ");
        initSucceed = false;
    } else {
        //  set IO port 
        pio.pinMode(PIO_CURRENT_ERRFLAG, INPUT);
        pio.pullUp(PIO_CURRENT_ERRFLAG, HIGH);  // turn on a 100K pullup internally

        pio.pinMode(PIO_CURRENT_ENABLE, OUTPUT);
        pio.digitalWrite(PIO_CURRENT_ENABLE, CURRENT_OFF);
    }

    //  ADコンバータ設定    PGA=x2   2.048V FS
    adconverter.begin();
    adconverter.setGain(GAIN_TWO); 

    Serial.println(" Fin. --"); 

    return initSucceed;

}

boolean Measurement::currentOn(void){
    boolean f_current_source_fail = true;

    Serial.print("currentCtrl:ON -- "); 
    pio.digitalWrite(PIO_CURRENT_ENABLE, CURRENT_ON);
    delay(10); // エラー判定が可能になるまで10ms待つ

    if (pio.digitalRead(PIO_CURRENT_ERRFLAG) == LOW){
        f_current_source_fail = true;
        pio.digitalWrite(PIO_CURRENT_ENABLE,CURRENT_OFF);
        Serial.print(" FAIL.  ");
    } else {
        f_current_source_fail = false;
        delay(100); // issue1: 電流のステイブルを待つ
        Serial.print(" OK.  ");
    }
    Serial.println("Fin. --");

    return !f_current_source_fail;
}

void Measurement::currentOff(void){
    Serial.print("currentCtrl:OFF  -- ");
    pio.digitalWrite(PIO_CURRENT_ENABLE, CURRENT_OFF);      
    Serial.println(" Fin. --");
}

void Measurement::setCurrent(uint16_t current){  // current in [0.1milliAmp]
    uint16_t value=0;

    Serial.print("currentSet: -- "); 
    if ( 670 < current && current < 830){
        value = (( current - 666 ) * DAC_COUNT_PER_VOLT) / CURRENT_SORCE_VI_COEFF;
        // current -> vref converting function
        current_adj_dac.setVoltage(value, false);
        Serial.print(" DAC changed. " ); 
      }
    Serial.println("Fin. --" ); 
}

boolean Measurement::getStatus(void){
    return (   (pio.digitalRead(PIO_CURRENT_ENABLE)==CURRENT_ON)    \
            && (pio.digitalRead(PIO_CURRENT_ERRFLAG) == HIGH)        \
    );

}

boolean Measurement::measSingle(void){
    
    boolean flag = false;
    Serial.println("singleShot: -- "); 
    

    if (!Measurement::currentOn()){
        flag = false;
        LevelMeter->setSensorError();
        Measurement::currentOff();
    } else {
        LevelMeter->clearSensorError();
        Serial.print("meas start..  ");

        //  センサへの熱伝導待ち時間の間に3回計測する（動いていますというフィードバックのため）
        for (uint16_t i =0 ; i < 3; i++){
            Measurement::readLevel();
            delay(delay_time/3);
        }
        //  確定値の計測
        Measurement::readLevel();

        Serial.println("single: meas end.");

        Measurement::currentOff();

        flag = true;
    }
        //  ---- FOR TEST
            Serial.print("meas start..  ");

            for (uint16_t i =0 ; i < 3; i++){
                Measurement::readLevel();
                delay(delay_time/3);
            }
            Measurement::readLevel();

            Serial.println("single: meas end.");

            Measurement::currentOff();
        //  ---- FOR TEST

    Serial.println("singleShot: Fin. --"); 

    return flag;
}

void Measurement::readLevel(void){
    uint32_t vout = Measurement::read_voltage(); // [micro Volt]
    uint32_t iout = Measurement::read_current(); // [micro Amp]

    // calc L-He level from the mesurement

    float ratio = 0.0;
    // 電流が計測されていない場合   0%  にする。
    if (iout != 0){
        Serial.print(" Resistance = "); Serial.println((float)vout/(float)iout);
        ratio = ((float)vout/(float)iout) / sensor_resistance ;
    } else { 
        Serial.print(" No current flow! ");
        ratio = 1.0;
    }
    
    uint16_t result = round(( 1.0 - ratio) * 1000);  // [0.1%]
    LevelMeter->setLiquidLevel(result);

}

uint32_t Measurement::read_voltage(void){  // return measured voltage of sensor in [microVolt]

    float results = 0.0;
    float readout = 0.0;
    uint16_t   avg = ADC_AVERAGE_DEFAULT;     // averaging

    if (DEBUG) {Serial.print("Voltage Meas: read_voltage(0-1): ");}

    for (uint16_t i = 0; i < avg; i++){
    //   results += (float)adconverter.readADC_Differential_0_1();
      readout = (float)(adconverter.readADC_Differential_0_1() - adc_OFS_comp_diff_0_1);
      if (DEBUG) {
          Serial.print(", "); Serial.print(readout);
          
      }
      results += readout;
    }

//    uint16_t gain_setting = ads.getGain();
    float adc_gain_coeff=0.0;
    switch (adconverter.getGain()){
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

    results = results / (float)avg * adc_gain_coeff * adc_err_comp_diff_0_1 * ATTENUATOR_COEFF;

// Lower limmit 
    if (results < 1.0){
        results = 1.0;
    }

    if (DEBUG) {
        Serial.print(results); Serial.println(" uV: Fin. --");
    }

    return round(results);
}

uint32_t Measurement::read_current(void){  // return measured current in [microAmp]

    float results = 0.0;
    uint16_t avg = ADC_AVERAGE_DEFAULT;  // averaging

    Serial.print("Current Meas: read_voltage(2-3): ");

    for (uint16_t i = 0; i < avg; i++){
      results += (float)(adconverter.readADC_Differential_2_3() - adc_OFS_comp_diff_2_3);
    }

    float adc_gain_coeff=0.0;
    switch (adconverter.getGain()){
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
    results = results / (float)avg * adc_gain_coeff * adc_err_comp_diff_2_3; // reading in microVolt
    results = results / (float)CURRENT_MEASURE_COEFF; // convert voltage to current.
    Serial.print(results);
    Serial.println(" uA: Fin. --");
    
  return round(results);
}

//  アナログレベルモニタ出力設定 [0.1%]
void Measurement::setVmon(uint16_t value){
    uint16_t da_value=0;

    //  100.0%以下の値ならそのまま設定、それ以外は更新しない
    if (value <= 1000) {
        da_value = ( DAC_COUNT_PER_VOLT * value ) / 1000;
    //     100.0% = 1V
        v_mon_dac.setVoltage(da_value, false);
    }

}