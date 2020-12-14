#ifndef _EH900_CLASS_H_
#define _EH900_CLASS_H_

#include <Adafruit_FRAM_I2C.h>

// モードの名前とその表示
enum Modes{Manual, Timer, Continuous};
const char ModeNames[3]={'M','T','C'};

//  センサ長の最大値、最小値（setterでのリミットに使用）
constexpr uint16_t SENSOR_LENGTH_MIN = 6;
constexpr uint16_t SENSOR_LENGTH_MAX = 24;

//  タイマー設定の最大値(setterでのリミットに使用)
constexpr uint16_t TIMER_PERIOD_MAX = 5400; // [s]

// FRAM ADDRESS:
//  （予備）フラグ用の領域(256byte)
constexpr uint16_t FRAM_FLAG_ADDR = 0x0000;

//  パラメタを保存する領域(256byte) Meter_parameters構造体をそのまま保存
constexpr uint16_t FRAM_PARM_ADDR = 0x0100;

//  液面表示の最大値[0.1%]
constexpr uint16_t LIQUID_LEVEL_UPPER_LIMIT = 1000;


struct Meter_parameters{
    
    //  装置のパラメタ

    //  センサ長 [inch]
    uint16_t sensor_length;
    //  タイマ設定  [s]
    uint16_t timer_period;
    //  現在のタイマ経過時間 [s]
    uint16_t timer_elasped;
    //  現在の液面  [0.1%]
    uint16_t liqud_level;


    //  計測ユニットのパラメタ

    //      ADコンバータのエラー補正系数    電圧計測
    float_t adc_err_comp_diff_0_1;
    //      ADコンバータのエラー補正系数    電流計測
    float_t adc_err_comp_diff_2_3;
    //      ADコンバータのオフセット補正    電圧計測
    int16_t adc_OFS_comp_diff_0_1;
    //      ADコンバータのオフセット補正    電流計測
    int16_t adc_OFS_comp_diff_2_3;

    //      電流源初期設定値    [0.1mA] 
    uint16_t current_set_default;

    //  センサエラーフラグ
    boolean f_sensor_error;
    boolean f_tick_tock;

    //  現在のモードを示す
    Modes   mode;
};



/*! @class eh900
    @brief  液面計のパラメタを保存する構造体 Meter_parameters のインスタンスを操作するためのクラス
*/
class eh900
{
    private:

        // パラメタ保存の構造体の実体 
        Meter_parameters eh_status = {};

        // 構造体の内容を保存するためのメモリのドライバ 
        Adafruit_FRAM_I2C fram;

        //  FRAMに変数の値を書き込む
        template <typename T> 
            const void nvram_put(uint16_t idx, const T& t);
        
        //  FRAMから変数に値を読み込む
        template <typename T> 
            void nvram_get(uint16_t, T&);

    public:
        eh900(void);
        
        // 液面計パラメタの初期化   FRAMから読み込んで設定
        boolean init(void);

        // 設定されているパラメタ(eh_status)をFRAMに書き込む 
        boolean storeParameter(void);

        // 設定されているセンサ長を返す[inch]
        uint16_t getSensorLength(void){
            return eh_status.sensor_length;
        };

        //  センサ長を設定する[inch]
        void setSensorLength(uint16_t);

        //  設定されているタイマ周期を返す[s]
        uint16_t getTimerPeriod(void){
            return eh_status.timer_period;
        };

        //  タイマ周期を設定する    最大5400秒(90分） 0の時タイマ動作停止
        void setTimerPeriod(uint16_t);

    //タイマ設定
        //  タイマの経過時間を返す
        uint16_t getTimerElasped(void){
            return eh_status.timer_elasped;
        };

        // タイマをインクリメント  timeupかどうかを返す true:timeUp
        boolean incTimeElasped(void);  

        // タイマの経過時間を設定
        void setTimerElasped(uint16_t);

    // 液面データ
        //  液面レベルを返す[0.1%単位]
        uint16_t getLiquidLevel(void){
            return eh_status.liqud_level;
        };
        
        //  液面レベルを設定[0.1%単位]
        void setLiquidLevel(uint16_t);


    //  モード

        //  モード設定
        void setMode(Modes mode){
            eh_status.mode = mode;
        };
    
        //  現在のモード読み取り
        Modes getMode(void){
            return eh_status.mode;
        };
        
    //  フラグ

        //  センサエラーかどうか
        boolean isSensorError(void){
            return eh_status.f_sensor_error;
        };
    
        //  センサエラーフラグを設定する
        void setSensorError(void){
            eh_status.f_sensor_error = true;
        };

        //  センサエラーフラグをクリアする
        void clearSensorError(void){
            eh_status.f_sensor_error = false;
        };

        boolean hasTickTock(void);

    //  計測ユニット用のデータ

        //  ADコンバータの補正係数を得る 電圧計測チャネル
        float_t getAdcErrComp01(void){
            return eh_status.adc_err_comp_diff_0_1;
        };

        //  ADコンバータの補正係数を設定 電圧計測チャネル
        void setAdcErrComp01(float_t value){
            if( 0.9 < value && value < 1.1 ){
                eh_status.adc_err_comp_diff_0_1 = value;
            }
        };

        //  ADコンバータの補正係数を得る 電流計測チャネル
        float_t getAdcErrComp23(void){
            return eh_status.adc_err_comp_diff_2_3;
        };

        //  ADコンバータの補正係数を設定 電流計測チャネル
        void setAdcErrComp23(float_t value){
            if( 0.9 < value && value < 1.1 ){
                eh_status.adc_err_comp_diff_2_3 = value;
            }
        };

        //  ADコンバータのオフセット補正値を得る 電圧計測チャネル
        int16_t getAdcOfsComp01(void){
            return eh_status.adc_OFS_comp_diff_0_1;
        };

        //  ADコンバータのオフセット補正値を設定 電圧計測チャネル
        void setAdcOfsComp01(int16_t value){
            if( -256 < value && value < 256 ){
                eh_status.adc_OFS_comp_diff_0_1 = value;
            }
        };

        //  ADコンバータのオフセット補正値を得る 電流計測チャネル
        int16_t getAdcOfsComp23(void){
            return eh_status.adc_OFS_comp_diff_2_3;
        };

        //  ADコンバータのオフセット補正値を設定 電流計測チャネル
        void setAdcOfsComp23(int16_t value){
            if( -256 < value && value < 256 ){
                eh_status.adc_OFS_comp_diff_2_3 = value;
            }
        };

        //  電流源設定値を得る  [0.1mA]
        uint16_t getCurrentSetting(void){
            return eh_status.current_set_default;
        };

        //  電流源設定値を設定  [Range: 67--83mA]
        void setCurrentSetting(uint16_t value){
            if ( 670 < value && value < 830){
                eh_status.current_set_default = value;
            }
        };
};

#endif // _EH900_CLASS_H_