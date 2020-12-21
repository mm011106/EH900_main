/*!
 * @file measurement.h
 */

#ifndef _MEASUREMENT_H_
#define _MEASUREMENT_H_

#include <Adafruit_ADS1015.h>   // ADC 16bit diff - 2ch
#include <Adafruit_MCP23008.h>  // PIO 8bit
#include <Adafruit_MCP4725.h>   // DAC  12bit 

#include "eh900_class.h"


/*!
 * @brief Class that stores state and functions for controlling measuement unit
 * 
 */

class Measurement {

    public:
        Measurement(eh900* pModel);

        // 計測モジュールのイニシャライズ
        //      液面計のパラメタが引数として必要
        boolean init(void);

    //  電流源制御
        // 電流源   ON  電流源が正常かどうかを返す(正常=true)
        boolean currentOn(void);
        // 電流源   OFF
        void currentOff(void);
        //  電流源を設定する  デフォルト：75mA  範囲：67mA〜83mA  [/0.1mA]
        void setCurrent(uint16_t current = 750);

        // 電流源が動作中かどうかチェックする   動作している=ture
        boolean getStatus(void);

    //  計測
        //液面を1回計測する   熱伝導速度も考慮に入れて時間待ちする 
        boolean measSingle(void);

        // 電流・電圧を計測して、センサ長で指定された規定のインピーダンスに対して何%なのかを0.1%単位で計算
        void readLevel(void);

        //  アナログレベルモニタ出力設定 [0.1%]
        void setVmon(uint16_t);

    private:
        //  電流設定用DAコンバータ
        Adafruit_MCP4725    current_adj_dac;
        //  アナログモニタ出力用DAコンバータ
        Adafruit_MCP4725    v_mon_dac;
        //  電流源制御用    GPIO
        Adafruit_MCP23008   pio;
        //  電圧・電流読み取り用ADコンバータ
        Adafruit_ADS1115    adconverter;

        //  液面計パラメタクラス
        eh900* LevelMeter;

        //  電圧・電流値の読み取り
        uint32_t read_voltage(void);
        uint32_t read_current(void);

        //  センサ抵抗値[ohm]
        float sensor_resistance = 0.0;
        //  熱伝導待ち時間 [ms]
        uint16_t delay_time = 0;

        //  センサエラーフラグ
        boolean f_sensor_error = false;
};

#endif // _MEASUREMENT_H_