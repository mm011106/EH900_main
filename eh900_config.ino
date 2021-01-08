/**************************************************************************/
/**
    @file   eh900_config.ino
    @brief  eh900 のパラメタを手動で変更するためのコンフィギュレーションメニューを提供
    @author miyamoto
    @date   2020/12/17
*/
/**************************************************************************/

#include "eh900_config.h"

// namespace{
//     //  センサ長の最大値、最小値（setterでのリミットに使用）
//     constexpr uint16_t SENSOR_LENGTH_MIN = 6;
//     constexpr uint16_t SENSOR_LENGTH_MAX = 24;
//     //  液面の上限値[0.1%]
//     constexpr uint16_t LIQUID_LEVEL_UPPER_LIMIT = 1000;

//     //  タイマー設定の最大値(setterでのリミットに使用)
//     constexpr uint16_t TIMER_PERIOD_MAX = 5400; // [s]
// }

/**
    @fn
        コンフィギュレーションメニューの１層目
    @brief  センサ長とタイマー時間のメニューを表示・選択させる
*/
void menu_main(void){
    // メニュー用の列挙型、メニュー名、カーソル位置の指定
    /**
     * @enum Enum
     * メニューの番号と内容の関連を示す
     */
    enum Menus{LengthConfig, TimerConfig, QuitConfig, Len_menu};

    // LCDに表示するメニュー名
    constexpr char* menu_names[]= {"LEN","TIM","QUIT"};

    // メニュー表示の位置
    constexpr uint16_t screen_position[Len_menu]={1,6,12};
    
    // スイッチの長押し判定時間の設定[ms]
    constexpr uint16_t DURATION_LONG_PRESS = 700; 

    // メニューから抜けるためのフラグ trueの時抜ける
    boolean f_exit = false;

    // 現在選択しているメニュー番号
    uint16_t menu = LengthConfig;


    lcd_display.clear();
    lcd_display.home();
    lcd_display.print("Config:");
    lcd_display.blink();

    for (uint16_t i=0; i<Len_menu; i++){
        lcd_display.setCursor(screen_position[i],1);
        lcd_display.print(menu_names[i]);
    }

    while (meas_sw.isDepressed()){
        meas_sw.updateStatus();
    }
    meas_sw.clearDuration();
    meas_sw.clearChangeStatus();

    while (! f_exit){

        meas_sw.updateStatus();
    
        lcd_display.setCursor(screen_position[menu]-1,1);

        if (meas_sw.getDuration() > DURATION_LONG_PRESS){
            lcd_display.flashDisplay();
        }

        if (meas_sw.hasReleased()) {
            if ( meas_sw.getDuration() > 700){

                switch(menu) {
                    case LengthConfig:
                        // change sensor length
                        meas_sw.clearDuration();
                        Serial.println("Length Config : ---");
                        menu_sensor_length();
                    break;

                    case TimerConfig:
                        // change timer duration
                        meas_sw.clearDuration();
                        Serial.println("Timer Config : ---");
                        menu_timer_period();
                    break;

                    case QuitConfig:        // quit menu
                        f_exit = true;
                    break;

                    default:
                        menu = LengthConfig;
                    break;
                }

                lcd_display.clear();
                lcd_display.home();
                lcd_display.print("Config:");
                lcd_display.blink();

                for (uint16_t i=0; i<Len_menu; i++){
                    lcd_display.setCursor(screen_position[i],1);
                    lcd_display.print(menu_names[i]);
                }

            } else {
                menu = ++menu % Len_menu;  
                Serial.print(menu); Serial.print(":");
            }
            meas_sw.clearDuration();
        }

        delay(50);
    }
    f_exit = false;
    lcd_display.noBlink();
    return;
}

/**
    @fn
        コンフィギュレーションメニューの２層目
    @brief  センサ長の設定をする
*/
void menu_sensor_length(void){
    lcd_display.clear();
    lcd_display.home();
    lcd_display.blink();
    lcd_display.print("Config: LENGTH");
    lcd_display.setCursor(10,1);
    lcd_display.print(" inch");

    //  カーソルの位置
    constexpr uint16_t CURSOR_POSITION = 8;

    // スイッチの長押し判定時間の設定[ms]
    constexpr uint16_t DURATION_LONG_PRESS = 700;

    // メニューから抜けるためのフラグ trueの時抜ける
    boolean f_exit = false;

    //  センサ長の設定のステップ[inch]
    uint16_t length_step = 2;

    uint16_t length = level_meter.getSensorLength();
    lcd_display.setCursor(CURSOR_POSITION, 1);
    lcd_display.print(right_align(String(length),2));
    lcd_display.setCursor(CURSOR_POSITION-2, 1);

    while (! f_exit ){

        meas_sw.updateStatus();

        if (meas_sw.getDuration() > DURATION_LONG_PRESS){
            lcd_display.flashDisplay();
        }

        if (meas_sw.hasReleased()) {
            if ( meas_sw.getDuration() > DURATION_LONG_PRESS){
                Serial.println("exit length menu");
                f_exit = true;
            } else {
                length = length + length_step;
                if (length > SENSOR_LENGTH_MAX){
                    length = SENSOR_LENGTH_MIN;
                }  
                Serial.print(length); Serial.print(":");
                lcd_display.setCursor(CURSOR_POSITION, 1);
                lcd_display.print(right_align(String(length),2));
                lcd_display.setCursor(CURSOR_POSITION-2, 1);
            }
            meas_sw.clearDuration();
        }
        
        delay(50);
    }
    
    level_meter.setSensorLength(length);

    return;
}


/**
    @fn
        コンフィギュレーションメニューの２層目
    
    @brief  タイマー周期の設定をする
            この関数内での時間の単位は[min.]で書き戻すときに[s]に変換している
*/
void menu_timer_period(void){
        
    lcd_display.clear();
    lcd_display.home();
    lcd_display.blink();
    lcd_display.print("Config: TIMER");
    lcd_display.setCursor(10,1);
    lcd_display.print(" min.");

    //  カーソルの位置  
    constexpr uint16_t CURSOR_POSITION = 8;

    // スイッチの長押し判定時間の設定[ms]
    constexpr uint16_t DURATION_LONG_PRESS = 700;
    
    //  タイマー周期時間設定のステップ[min.]
    constexpr uint16_t timer_step = 10; 

    // メニューから抜けるためのフラグ trueの時抜ける
    boolean f_exit = false;

    uint16_t timer_period = level_meter.getTimerPeriod()/60; // [min.]
    lcd_display.setCursor(CURSOR_POSITION, 1);
    lcd_display.print(right_align(String(timer_period),2));
    lcd_display.setCursor(CURSOR_POSITION-2, 1);
    
    while (! f_exit ){

        meas_sw.updateStatus();

        if (meas_sw.getDuration() > DURATION_LONG_PRESS){
            lcd_display.flashDisplay();
        }

        if (meas_sw.hasReleased()) {
            if ( meas_sw.getDuration() > DURATION_LONG_PRESS){
                Serial.println("exit length menu");
                f_exit = true;
            } else {
                timer_period = timer_period + timer_step;
                if (timer_period > TIMER_PERIOD_MAX/60){
                    timer_period = 0;
                }
                Serial.print(timer_period); Serial.print(":");
                lcd_display.setCursor(CURSOR_POSITION, 1);
                lcd_display.print(right_align(String(timer_period),2));
                lcd_display.setCursor(CURSOR_POSITION-2, 1);
            }
            meas_sw.clearDuration();
        }
        
        delay(50);
    }
    
    level_meter.setTimerPeriod(timer_period*60);

    return;
}