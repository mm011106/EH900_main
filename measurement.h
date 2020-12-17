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
        Measurement(void);

        // 計測モジュールのイニシャライズ
        //      液面計のパラメタが引数として必要
        boolean init(eh900* pModel);

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

        eh900* LevelMeter;


        //  電圧・電流値の読み取り
        uint32_t read_voltage(void);
        uint32_t read_current(void);

        //  センサ抵抗値
        float sensor_resistance = 0.0;
        //  熱伝導待ち時間 
        uint16_t delay_time = 0;
        
        //  ADコンバータ ch0-1  ゲイン誤差補正
        float adc_err_comp_diff_0_1=1.0;
        //  ADコンバータ ch2-3  ゲイン誤差補正
        float adc_err_comp_diff_2_3=1.0;
        //  ADコンバータ ch0-1  オフセット誤差補正[LSB]
        int16_t adc_OFS_comp_diff_0_1=0;
        //  ADコンバータ ch2-3  オフセット誤差補正[LSB]
        int16_t adc_OFS_comp_diff_2_3=0;

        //  電流源の初期設定値 [0.1mA] 
        uint16_t current_source_default = 750;


};

//  I2C adress 
constexpr uint16_t I2C_ADDR_ADC            = 0x48;
constexpr uint16_t I2C_ADDR_CURRENT_ADJ    = 0x60;
constexpr uint16_t I2C_ADDR_V_MON          = 0x61;
constexpr uint16_t I2C_ADDR_PIO            = 0x20;


// ADの読み値から電圧値を計算するための系数 [/ micro Volts/LSB]
// 3.3V電源、差動計測（バイポーラ出力）を想定
constexpr float ADC_READOUT_VOLTAGE_COEFF_GAIN_TWOTHIRDS    = 187.506;  //  FS 6.144V * 1E6/32767
constexpr float ADC_READOUT_VOLTAGE_COEFF_GAIN_ONE          = 125.004;  //  FS 4.096V * 1E6/32767
constexpr float ADC_READOUT_VOLTAGE_COEFF_GAIN_TWO          = 62.5019;  //  FS 2.048V * 1E6/32767
constexpr float ADC_READOUT_VOLTAGE_COEFF_GAIN_FOUR         = 31.2509;  //  FS 1.024V * 1E6/32767
constexpr float ADC_READOUT_VOLTAGE_COEFF_GAIN_EIGHT        = 15.6255;  //  FS 0.512V * 1E6/32767
constexpr float ADC_READOUT_VOLTAGE_COEFF_GAIN_SIXTEEN      = 7.81274;  //  FS 0.256V * 1E6/32767

// 電流計測時の電流電圧変換係数(R=5kohm, Coeff_Isenosr=0.004の時) [/ V/A]
constexpr float CURRENT_MEASURE_COEFF = 20.075;

//  電圧計測のアッテネータ系数  1/10 x 2/5 の逆数  実際の抵抗値での計算
constexpr float ATTENUATOR_COEFF = 24.6642;

//  センサの熱伝導速度  測定待ち時間の計算に必要
constexpr float HEAT_PROPERGATION_VEROCITY = 7.9; // inch/s

// AD変換時の平均化回数 １回測るのに10msかかるので注意  10回で100ms
constexpr uint16_t ADC_AVERAGE_DEFAULT = 10;

//  PIOのポート番号の設定と論理レベル設定
constexpr uint16_t PIO_CURRENT_ENABLE    = 4 ;  // ON = LOW,  OFF = HIGH
constexpr uint16_t PIO_CURRENT_ERRFLAG   = 0 ;  // LOW = FAULT(short or open), HIGH = NOMAL
#define CURRENT_OFF       (HIGH)
#define CURRENT_ON        (LOW)

//  電流源設定用DAC MCP4725 1Vあたりの電流[0.1mA]  A/V
constexpr uint16_t  CURRENT_SORCE_VI_COEFF  = 56;

//  DAC MCP4275の1Vあたりのカウント (3.3V電源にて） COUNT/V
constexpr uint16_t DAC_COUNT_PER_VOLT = 1241;

#endif // _MEASUREMENT_H_