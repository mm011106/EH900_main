#include <Arduino.h>
#include <HardwareTimer.h>

#include "switch_class.h"
#include "eh900_class.h"
#include "measurement.h"
#include "display_class.h"
#include "eh900_config.h"

//  スイッチのポート指定
constexpr uint16_t MEAS_SWITCH = D3;    
// 長押しを判定する時間[ms]
constexpr uint16_t DURATION_LONG_PRESS = 2000; 
//  １秒のタイマ設定値（調整込み）  [us]
constexpr uint32_t ONE_SECOND = 999025; 
//  手動計測時の表示アップデート周期[us]
constexpr uint32_t UPDATE_CYCLE =300000;    
//  連続計測時のデシメーション（10回ループを回ったら1回計測）
constexpr uint16_t DECIMATION = 10; 
//  ループ1回ごとの時間待ち[ms] 実際のループ１周は  この時間＋処理時間（600us）
constexpr uint16_t LOOP_WAIT = 100; 


constexpr boolean DEBUG = false;  // デバグフラグ

eh900 level_meter;
Measurement meas_unit(&level_meter);
Eh_display lcd_display(&level_meter);

Switch meas_sw(MEAS_SWITCH);


//  手動計測時の表示アップデート用タイマ
HardwareTimer* disp_update_timer = new HardwareTimer(TIM1);

//  １秒のクロック作成タイマ
HardwareTimer* tick_tock_timer = new HardwareTimer(TIM3);
boolean f_tick_tock = false;

uint16_t deci_counter = 0;      //  連続計測の時のループ回数カウント
uint16_t system_error = 0;      //  起動時のエラーコード    
                                //      0:ok 1:設定MEMORY 2:計測ユニット 4:表示 
boolean f_timer_timeup=false;   //  計測タイマー用フラグ

void setup() {
    Serial.begin(115200);
    Serial.println("INIT:--");

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(D12,OUTPUT);
    digitalWrite(D12,LOW);

    Serial.print("Switch : "); Serial.println(meas_sw.getPortID());
    attachInterrupt(digitalPinToInterrupt(meas_sw.getPortID()), isr_warpper_meas_sw , CHANGE);

    Serial.print("Memory : "); 
    if (level_meter.init()){
        Serial.print(" -- OK ");
    } else {
        system_error |= 1;
        //  メモリがない時の液面計パラメタの初期化
        level_meter.setSensorLength(20);
        level_meter.setTimerPeriod(1800);
        level_meter.setMode(Timer);
        level_meter.setAdcErrComp01(1.0);
        level_meter.setAdcErrComp23(1.0);
        level_meter.setAdcOfsComp01(0);
        level_meter.setAdcOfsComp23(0);
        level_meter.setCurrentSetting(750);
    };
    Serial.println(system_error);

    Serial.print("Meas. Unit : "); 
    if (meas_unit.init()){
        Serial.println(" -- OK ");
    } else {
        Serial.println(" -- Fail..");
        system_error |= 2;
    };
    Serial.println(system_error);

    //  画面初期化  型名の表示・エラー表示
    Serial.println("Disp : "); 
    if (!lcd_display.init(system_error)){
        Serial.println("  Disp init error.");
        system_error |= 4;
    };

    if (DEBUG){ 
        Serial.println("DEBUG MODE!!!");
        system_error = 0;
    };

    //  メモリか測定ユニットにエラーがあれば起動しない。
    while(system_error != 0){
        delay(1000);
    };

    Serial.println("Timer : "); 
    //    手動計測時の液面表示アップデート用タイマ
    disp_update_timer -> pause();
    disp_update_timer -> setOverflow(UPDATE_CYCLE , MICROSEC_FORMAT); 
    disp_update_timer -> refresh();
    disp_update_timer -> attachInterrupt(isr_disp_update);

    //  1s  タイマ  計測タイマーの動作カウント用
    tick_tock_timer -> pause();
    tick_tock_timer -> setOverflow(ONE_SECOND, MICROSEC_FORMAT); 
    tick_tock_timer -> refresh();
    tick_tock_timer -> attachInterrupt(isr_tick_tock);


    //  設定モードへ移行するスイッチ操作の完了待ち[3s]
    delay(3000);

    //  この時点でスイッチが押されていれば設定モードへ移行
    if (meas_sw.isDepressed()){
        menu_main();
        iinfo(0);

        lcd_display.noDisplay();
        Serial.print("QUIT config: store parameters into FRAM...");
        level_meter.setTimerElasped(0);
        level_meter.setLiquidLevel(0);
        level_meter.clearSensorError();
        //  変更されたパラメタを保存
        level_meter.storeParameter();        
        
        delay(500);
        lcd_display.display();
        // センサ長から計算されるパラメタを再初期化
        meas_unit.renew_sensor_parameter();
    }

    Serial.print("Level Meter:"); Serial.print((uint32_t)&level_meter,HEX); Serial.print("/");Serial.println(sizeof(level_meter));
    Serial.print("Meas Unit:"); Serial.print((uint32_t)&meas_unit,HEX); Serial.print("/");Serial.println(sizeof(meas_unit));
    Serial.print("Meas_sw:"); Serial.print((uint32_t)&meas_sw,HEX); Serial.print("/");Serial.println(sizeof(meas_sw));
    Serial.print("LCD-display:"); Serial.print((uint32_t)&lcd_display,HEX); Serial.print("/");Serial.println(sizeof(lcd_display));

    iinfo(0);

    Serial.println("");
    Serial.println("START : ---");
  
    //  計測モードのために画面初期化
    lcd_display.showMeter();
    lcd_display.showMode();
    lcd_display.showTimer();
    lcd_display.showLevel();

    //測定用タイマ  動作開始
    tick_tock_timer -> resume(); 
}

// 100ms＋処理時間  でループ 2msぐらいの仕事量
void loop() {
    digitalWrite(D12,HIGH); // 動作時間測定
    //  モード表示リフレッシュ
    lcd_display.showMode();
    lcd_display.showTimer();

    //  連続モードのとき
    if (level_meter.getMode() == Continuous ){

        // スイッチが離されていれば
        if(meas_sw.isReleased()){
            //  タイマー動作をしないようにする
            f_timer_timeup = false;
            ++deci_counter;
            // DECIMATION 回ごとに1回計測
            if (deci_counter == DECIMATION - 1 ){
                Serial.print("-");
                //  電流源の動作確認
                if ( meas_unit.getStatus() ){
                    //  動作していれば  1回計測、表示
                    level_meter.clearSensorError();
                    meas_unit.readLevel();
                    lcd_display.showLevel();
                    meas_unit.setVmon(level_meter.getLiquidLevel());
                    deci_counter = 0;
                } else {
                    //  動作していなければ計測をターミネート
                    meas_unit.currentOff();
                    digitalWrite(LED_BUILTIN, LOW);
                    // エラー表示
                    level_meter.setSensorError();
                    lcd_display.showLevel();
                    meas_unit.setVmon(level_meter.getLiquidLevel());
                    // タイマーモードに移行
                    level_meter.setMode(Timer);
                    Serial.println("  Current Sorce Fail. Cont meas terminated...");
                }
            }
        }
        
        if(meas_sw.hasDepressed() ){
            //  連続計測モードでスイッチが押されたら
            //  タイマーモードへ移行して、測定を終了する
            meas_unit.currentOff();
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
            level_meter.setMode(Manual);
            wrapper_meas_single();
            lcd_display.showLevel();
            meas_unit.setVmon(level_meter.getLiquidLevel());
            level_meter.setMode(Timer);
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

        //  スイッチ状態のモニタ
        // Serial.print(meas_sw.getDuration()); Serial.print(":");
        // Serial.println(ModeNames[level_meter.getMode()]);

        switch (level_meter.getMode()){
            case Manual:    //1回計測を実行して完了

                lcd_display.showMode();
                wrapper_meas_single();
                lcd_display.showLevel();
                meas_unit.setVmon(level_meter.getLiquidLevel());
                // 手動計測中にタイマがタイムアップした場合、それを無視する
                f_timer_timeup = false;
                //  測定している間のスイッチ操作を無視
                meas_sw.clearChangeStatus();
                
                break;
        
            case Continuous:    // 連続計測モードへ移行

                Serial.print("Cont. Measureing... ");
                digitalWrite(LED_BUILTIN, HIGH);
                lcd_display.showMode();
                //  電流をon
                if ( !meas_unit.currentOn() ){ 
                    //  電流源にエラーがあればエラー表示してタイマーモードへ移行
                    level_meter.setSensorError();
                    lcd_display.showLevel();
                    level_meter.setMode(Timer);
                } else {
                    level_meter.clearSensorError();
                    lcd_display.showLevel();
                }

                break;

            default:
                level_meter.setMode(Timer);
                lcd_display.showMode();
                
                break;
        }
    }

    // if (DEBUG) { 
        digitalWrite(D12,LOW); 
    // }

    delay(LOOP_WAIT);
}

//  1回計測 measurement::measSingleのラッパ
void wrapper_meas_single(void){
    Serial.print("Single shot Measureing...");
    digitalWrite(LED_BUILTIN, HIGH);  // turn the LED
    lcd_display.showMode();

    Serial.print("timer start.. ");

    disp_update_timer -> resume();//    表示リフレッシュ用タイマ動作開始
    if (!meas_unit.measSingle()){
        level_meter.setSensorError();
    } else {
        level_meter.clearSensorError();
    }
    disp_update_timer -> pause();   //  表示リフレッシュ用タイマ動作終了
    disp_update_timer -> refresh(); //      同  リセット

    Serial.println("  Finished.");

    digitalWrite(LED_BUILTIN, LOW);  
    level_meter.setMode(Timer);
    lcd_display.showMode();
}

//  スイッチ操作のISR  スイッチクラスのラッパ 
void isr_warpper_meas_sw(){    
    meas_sw.read_switch_status();
    Serial.print("!");
}

// 液面表示アップデート用 ISR
void isr_disp_update(void){  
    lcd_display.showLevel();
    Serial.print("@"); // means 'measureing'
    if (DEBUG){ iinfo(1); };
}

// 毎秒のタイマー ISR
void isr_tick_tock(void){  
    if (DEBUG){
        iinfo(1); 
    };

    //  タイマ設定が0ならカウントしない：タイマ計測はしない
    if (level_meter.getTimerPeriod() == 0){
        return;
    }

    if (level_meter.incTimeElasped()) {
        f_timer_timeup = true;
    };
}

// メモリ利用状況の確認
void iinfo(uint8_t mode) {
    char top = 't';
    uint32_t adr = (uint32_t)&top;
    uint8_t* tmp = (uint8_t*)malloc(1);
    uint32_t hadr = (uint32_t)tmp;
    free(tmp);

    if (mode==0){
        Serial.print("Stack Top:"); Serial.println(adr,HEX);
        Serial.print("Heap Top :"); Serial.println(hadr,HEX);
        }
    // SRAM未使用領域の表示
    Serial.print("SRAM Free:"); Serial.println(adr-hadr,DEC);
}