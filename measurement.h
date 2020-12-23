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
 * @brief Class that stores state and functions for controlling measuement module
 * 
 */
class Measurement {

    public:

        Measurement(eh900* pModel) : LevelMeter(pModel) {
 
        }

        ~Measurement(){
            Serial.println("~ Mesasurement ----");
            if(current_adj_dac){ delete current_adj_dac; }
            if(v_mon_dac){ delete v_mon_dac; }
            if(pio){ delete pio; }
            if(adconverter){ delete adconverter; }
        }
    //  初期化
    
        boolean init(void);

    //  電流源制御
 
        boolean currentOn(void);
        void currentOff(void);
        void setCurrent(uint16_t current = 750);
        boolean getStatus(void);

    //  計測

        boolean measSingle(void);
        void readLevel(void);

    //  モニタ出力制御
    
        void setVmon(uint16_t);

    private:
        //  電流設定用DAコンバータ
        Adafruit_MCP4725*    current_adj_dac = nullptr;
        //  アナログモニタ出力用DAコンバータ
        Adafruit_MCP4725*    v_mon_dac = nullptr;
        //  電流源制御用    GPIO
        Adafruit_MCP23008*   pio = nullptr;
        //  電圧・電流読み取り用ADコンバータ
        Adafruit_ADS1115*    adconverter = nullptr;

        //  液面計パラメタクラス
        eh900* LevelMeter = nullptr;

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