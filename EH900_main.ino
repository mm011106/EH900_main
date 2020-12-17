#include <Arduino.h>
#include <HardwareTimer.h>

#include "switch_class.h"
#include "eh900_class.h"
#include "measurement.h"
#include "display_class.h"
#include "eh900_config.h"

constexpr uint16_t MEAS_SWITCH = D3;    //  スイッチのポート指定
constexpr uint16_t DURATION_LONG_PRESS = 2000; // 長押しを判定する時間[ms]
constexpr uint32_t ONE_SECOND = 999025; //  １秒のタイマ設定値（調整込み）  [us]
constexpr uint16_t DECIMATION = 10; //  連続計測時のデシメーション（10回ループを回ったら1回計測）

constexpr boolean DEBUG = true;  // デバグフラグ

Switch meas_sw(MEAS_SWITCH);

Eh_display lcd_display;

eh900 level_meter;
Measurement meas_uint;

HardwareTimer* disp_update_timer = new HardwareTimer(TIM1);

HardwareTimer* tick_tock_timer = new HardwareTimer(TIM3);
boolean f_tick_tock = false;

uint16_t deci_counter = 0;      //  連続計測の時のループ回数カウント
uint16_t system_error = 0;      //  起動時のエラーコード    
                                //      0:ok 1:設定MEMORY 2:計測ユニット
boolean f_timer_timeup=false;   //  計測タイマー用フラグ

void setup() {
  // initialize digital pin LED_BUILTIN as an output.
    Serial.begin(115200);
    Serial.println("INIT:--");

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(D12,OUTPUT);
    digitalWrite(D12,LOW);

    Serial.print("Switch : "); Serial.println(meas_sw.getPortID());
    attachInterrupt(digitalPinToInterrupt(meas_sw.getPortID()), isr_warp_meas_sw , CHANGE);

    Serial.print("Memory : "); 
    if (level_meter.init()){
        Serial.print(" -- OK ");
    } else {
        system_error |= 1;
        level_meter.setSensorLength(6);
        level_meter.setTimerPeriod(60);
        level_meter.setMode(Timer);
        level_meter.setAdcErrComp01(1.0);
        level_meter.setAdcErrComp23(1.0);
        level_meter.setAdcOfsComp01(0);
        level_meter.setAdcOfsComp23(0);
        level_meter.setCurrentSetting(750);
    };

    Serial.println(system_error);

    
    Serial.print("Meas. Unit : "); 
    if (meas_uint.init(&level_meter)){
        Serial.println(" -- OK ");
    } else {
        Serial.println(" -- Fail..");
        system_error |= 2;
    };
    Serial.println(system_error);

    if (DEBUG){ 
        Serial.println("DEBUG MODE!!!");
        system_error = 0;
    };

    Serial.println("Disp : "); 
    lcd_display.init(&level_meter, system_error);

    //  メモリか測定ユニットにエラーがあれば起動しない。
    while(system_error != 0){
        delay(1000);
    };

    Serial.println("Timer : "); 

    //  100ms タイマ  手動計測時の液面表示アップデート用
    disp_update_timer -> pause();
    disp_update_timer -> setOverflow(100000 , MICROSEC_FORMAT); 
    disp_update_timer -> refresh();
    disp_update_timer -> attachInterrupt(isr_disp_update);

    //  1s  タイマ  計測タイマーの動作カウント用
    tick_tock_timer -> pause();
    tick_tock_timer -> setOverflow(ONE_SECOND, MICROSEC_FORMAT); 
    tick_tock_timer -> refresh();
    tick_tock_timer -> attachInterrupt(isr_tick_tock);
    tick_tock_timer -> resume();

    // Serial.println(disp_update_timer -> getTimerClkFreq());
    // Serial.println(tick_tock_timer -> getTimerClkFreq());

    Serial.println("");
    Serial.println("START : ---");

    delay(3000);

    if (meas_sw.isDepressed()){
        menu_main();
        
        lcd_display.noDisplay();
        Serial.print("QUIT config: store parameters into FRAM...");
        level_meter.setTimerElasped(0);
        level_meter.setLiquidLevel(0);
        level_meter.clearSensorError();

        level_meter.storeParameter();        
        
        delay(500);
        lcd_display.display(); 
    }
    
    lcd_display.showMeter();
    lcd_display.showMode();
    lcd_display.showTimer();
    lcd_display.showLevel();

}

// 100ms＋処理時間  でループ 580usぐらいの仕事量
void loop() {
    if (DEBUG) { digitalWrite(D12,HIGH); }
    //  モード表示リフレッシュ
    lcd_display.showMode();
    lcd_display.showTimer();

    //  連続モードのとき
    if (level_meter.getMode() == Continuous){

        //  タイマー動作をしないようにする
        f_timer_timeup = false;
        // DECIMATION 回ごとに1回計測  適度に調整
        ++deci_counter;
        // if ( (deci_counter == DECIMATION -1) && meas_unit.getStatus()){ // 電流源が動作していれば計測
        
        
        if (deci_counter == DECIMATION -1 ){
            Serial.print("-");
            if (!DEBUG) {
                //  電流源の動作確認
                if ( !meas_uint.getStatus() ){
                    //  動作していなければ
                    digitalWrite(LED_BUILTIN, LOW);  
                    level_meter.setMode(Timer);
                    Serial.println("  Current Sorce Fail. Cont meas terminated...");
                }
            }
            //  1回計測、表示
            meas_uint.readLevel();
            lcd_display.showLevel();
            meas_uint.setVmon(level_meter.getLiquidLevel());
            deci_counter = 0;
        }

        if(meas_sw.hasDepressed() ){ // 連続計測モードでスイッチが押されたら
            //  連続計測モードからタイマーモードへ移行して、測定を終了する
            digitalWrite(LED_BUILTIN, LOW);  
            level_meter.setMode(Timer); 
            //  スイッチが離されていることを確認して完了
            while (! meas_sw.hasReleased()){
                meas_sw.updateStatus();
            }
            meas_sw.clearChangeStatus();
            Serial.println("  Finished.");
        }    
    }

    //  タイマモードの時
    if (level_meter.getMode() == Timer){

        if (f_timer_timeup) {  //  タイムアップが起きれば計測する
            f_timer_timeup = false;
            Serial.print("Timer UP - ");
            dummy_meas_single();
            lcd_display.showLevel();
            meas_uint.setVmon(level_meter.getLiquidLevel());
        }
    }
    
    // スイッチの状態をサンプリング
    meas_sw.updateStatus();

    //  スイッチが押されていれば、測定モードの変更
    if (meas_sw.isDepressed()){       // スイッチが押されている時、
        meas_sw.clearChangeStatus();
        digitalWrite(LED_BUILTIN, HIGH);

        // lcd.setCursor(0, 1);
        if(meas_sw.getDuration() > DURATION_LONG_PRESS){   //押されている時間が規定より長ければCモード
            // lcd.print("C");
            level_meter.setMode(Continuous);
        } else {                                    //そうでなければMモード
            level_meter.setMode(Manual);
        }
    } 

    //  スイッチが離された時  モード毎のコードを実行  
    if (meas_sw.hasReleased()){
        meas_sw.clearChangeStatus();

        if (DEBUG){
            Serial.print(meas_sw.getDuration()); Serial.print(":");
            Serial.println(ModeNames[level_meter.getMode()]);
        }

        switch (level_meter.getMode()){
            case Manual:    //1回計測を実行して完了
                lcd_display.showMode();
                dummy_meas_single();
                lcd_display.showLevel();
                meas_uint.setVmon(level_meter.getLiquidLevel());
                // 手動計測中にタイマがタイムアップした場合、それを無視する
                f_timer_timeup = false;
                //  測定している間のスイッチ操作を無視
                meas_sw.clearChangeStatus();
                
                break;
        
            case Continuous:    // 連続計測モードへ移行
                Serial.print("Cont. Measureing... ");
                digitalWrite(LED_BUILTIN, HIGH);
                lcd_display.showMode();

                if (!DEBUG){
                    //  電流をon
                    if ( ! meas_uint.currentOn() ){ 
                        //  電流源にエラーがあればエラー表示してタイマーモードへ移行
                        level_meter.setSensorError();
                        level_meter.setMode(Timer);
                    }
                }

                break;

            default:
                level_meter.setMode(Timer);
                lcd_display.showMode();
                
                break;
        }
    }

    if (DEBUG) { digitalWrite(D12,LOW); }

    delay(100);
}

void dummy_meas_single(void){
    Serial.print("Single shot Measureing...");
    digitalWrite(LED_BUILTIN, HIGH);  // turn the LED
    lcd_display.showMode();

    Serial.print("timer start.. ");

    disp_update_timer -> resume();//    表示リフレッシュ用タイマ動作開始
    if (!DEBUG) {
        meas_uint.measSingle();
    } else{
        delay(3000);
    }
    disp_update_timer -> pause();   //  表示リフレッシュ用タイマ動作終了
    disp_update_timer -> refresh(); //      同  リセット

    Serial.println("  Finished.");

    digitalWrite(LED_BUILTIN, LOW);  
    level_meter.setMode(Timer);
    lcd_display.showMode();
}

void isr_warp_meas_sw(){    //  スイッチ操作のISR  スイッチクラスのラッパ 
    meas_sw.read_switch_status();
    Serial.print("!");
}

void isr_disp_update(void){  // 液面表示アップデート用 ISR
    lcd_display.showLevel();
    Serial.print("@"); // means 'measureing'
}

void isr_tick_tock(void){   // 毎秒のタイマー ISR
    if (level_meter.incTimeElasped()) {
        f_timer_timeup = true;
    };
    if (DEBUG){ iinfo(); };
}

void iinfo() {
    char top = 't';
    uint32_t adr = (uint32_t)&top;
    uint8_t* tmp = (uint8_t*)malloc(1);
    uint32_t hadr = (uint32_t)tmp;
    free(tmp);

//   // スタック領域先頭アドレスの表示
//   Serial.print("Stack Top:"); Serial.println(adr,HEX);
  
//   // ヒープ領域先頭アドレスの表示
//   Serial.print("Heap Top :"); Serial.println(hadr,HEX);

//   // SRAM未使用領域の表示
  Serial.print("SRAM Free:"); Serial.println(adr-hadr,DEC);
}