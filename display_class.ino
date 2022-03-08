#include "display_class.h"

namespace{
    //  表示データとカーソル位置の定数
    constexpr uint16_t POSITION_BAR_GRAPH       = 10;
    constexpr uint16_t POSITION_SENSOR_LENGTH   = 1;
    constexpr uint16_t POSITION_TIMER_SET       = 5;
    constexpr uint16_t POSITION_TIMER_COUNT     = 2;
    constexpr uint16_t POSITION_LEVEL           = 10;
    constexpr uint16_t POSITION_MODE            = 0;

    //  バーグラフのためのCGデータ
    byte bar_graph[5][8] = {
        {
            0b10000,
            0b10000,
            0b10000,
            0b10000,
            0b10000,
            0b10000,
            0b10000,
            0b10000 
        },
        {
            0b11000,
            0b11000,
            0b11000,
            0b11000,
            0b11000,
            0b11000,
            0b11000,
            0b11000
        },
        {
            0b11100,
            0b11100,
            0b11100,
            0b11100,
            0b11100,
            0b11100,
            0b11100,
            0b11100
        },   
        {
            0b11110,
            0b11110,
            0b11110,
            0b11110,
            0b11110,
            0b11110,
            0b11110,
            0b11110
        },   
        {
            0b11111,
            0b11111,
            0b11111,
            0b11111,
            0b11111,
            0b11111,
            0b11111,
            0b11111
        },
    };

}

/*! 
    @brief  コンストラクタ
    @param pModel eh900型のインスタンスのポインタ（表示するパラメタの参照用）
*/
Eh_display::Eh_display(eh900* pModel) : LevelMeter(pModel) {
 }

/*! 
    @brief  LCDイニシャライズ、スプラッシュ表示、エラー表示
    @param error エラーコード（エラー表示のため）
    @return True:正常終了   False:パラメタ参照用のeh900型変数が未定義
*/
boolean Eh_display::init(uint16_t error){

    if (!LevelMeter){
        Serial.println("Eh_display::int Parameter is null.");
        return false;
    }

    rgb_lcd::begin(16,2);

    for (int i=0; i<5; i++){
        rgb_lcd::createChar(i, bar_graph[i]);
    }

    rgb_lcd::clear();
    rgb_lcd::setCursor(2, 0);

    rgb_lcd::print("-- EH-900 --");
    if(error != 0){
        rgb_lcd::setCursor(3, 1);
        rgb_lcd::print("INIT ERR:");
        rgb_lcd::print(right_align(String( error ), 2));
    }
    return true;
}

/*!
    @brief  メータの基本フォーマットを表示
*/
void Eh_display::showMeter(void){
        rgb_lcd::setCursor(0, 0);
        rgb_lcd::print(" :  /   E:    :F");

        rgb_lcd::setCursor(POSITION_TIMER_SET,0);
        rgb_lcd::print(right_align(String( LevelMeter->getTimerPeriod() / 60 ), 2));

        rgb_lcd::setCursor(POSITION_SENSOR_LENGTH,1);
        rgb_lcd::print(right_align(String( LevelMeter->getSensorLength() ), 2));
        rgb_lcd::print("inch   ");
}

/*!
    @brief  液面の表示（数値・バーグラフ）、センサエラーの表示
*/
void Eh_display::showLevel(void){
    uint16_t value = LevelMeter->getLiquidLevel();

    delay(10);
    rgb_lcd::setCursor(POSITION_LEVEL, 1);
    rgb_lcd::print(right_align(String((float)value/10.0,1),5));
    rgb_lcd::print("%");


    rgb_lcd::setCursor(POSITION_BAR_GRAPH, 0);
    // clear bar display area
    rgb_lcd::print("    ");

    uint16_t fine = (value % 250)/50;
    uint16_t coarse = value / 250;
    uint16_t x_pos = 0;

    // block of 25%
    for (x_pos=0; x_pos<coarse; x_pos++){
        rgb_lcd::setCursor(POSITION_BAR_GRAPH+x_pos,0);
        rgb_lcd::write((unsigned char)4);
    };
    // block of 5%
    if (fine>0){
        rgb_lcd::setCursor(POSITION_BAR_GRAPH+x_pos,0);
        rgb_lcd::write((unsigned char)(fine-1));
    };

    //  センサエラー表示
    if(LevelMeter->isSensorError()){
        rgb_lcd::setCursor(POSITION_SENSOR_LENGTH,1);
        rgb_lcd::print("-ERROR-");
    } else {
        rgb_lcd::setCursor(POSITION_SENSOR_LENGTH,1);
        rgb_lcd::print(right_align(String( LevelMeter->getSensorLength() ), 2));
        rgb_lcd::print("inch   ");
    }
    
}
/*!
    @brief  モードの表示、セミコロンおよびモード表示のフラッシュ含む
    @details 比較的短い周期（100ms程度）で周期的に呼ぶことでスムースに表示
*/
void Eh_display::showMode(void){

    // rgb_lcd::setCursor(POSITION_MODE,0);

    // 連続モードの時にフラッシュする   1sec周期でブリンク
    if ( ( LevelMeter->getMode() == Continuous ) && ( millis()%1000 < 500 )){
        rgb_lcd::setCursor(POSITION_MODE,0);
        rgb_lcd::print(" ");
    } else {
        rgb_lcd::setCursor(POSITION_MODE,0);
        rgb_lcd::print(ModeNames[LevelMeter->getMode()]);
    }      

    // タイマーモードの時のtick-tock 
    rgb_lcd::setCursor(POSITION_MODE+1,0);
    // 2sec周期でブリンク
    if ( ( LevelMeter->getMode() == Timer ) && ( millis()%2000 < 1000 ) && !(LevelMeter->getTimerPeriod()==0)){
        rgb_lcd::write(" ");
    } else {
        rgb_lcd::write(":");
    }
    rgb_lcd::setCursor(POSITION_MODE,0);

}

/*!
    @brief  タイマーの時間経過を表示
*/
void Eh_display::showTimer(void){
    rgb_lcd::setCursor(POSITION_TIMER_COUNT,0);
    rgb_lcd::print(right_align(String(LevelMeter->getTimerElasped() / 60),2));
    rgb_lcd::setCursor(POSITION_MODE,0);
}

/*!
    @brief  ディスプレイ全体をブリンクさせる．300msかかる
*/
void Eh_display::flashDisplay(void){
    constexpr uint16_t interval = 200;
    rgb_lcd::noDisplay();
    delay(interval + 100);
    rgb_lcd::display();
    delay(interval);
}
/*!
 * @brief 文字列を指定された桁数の右詰にする
 * @param num_in_string 文字列
 * @param digit 桁数
 * @returns 右詰にされた文字列
 */
String right_align(const String num_in_string, const uint16_t digit){

    String right_aligned="";
    String pad="";

    if (digit>1){
        //  string pad(digit-1, ' '); と同じ  arduino のstringはこれができない
        for (uint16_t i=0; i< digit-1; i++){
            pad += " ";
        }

        pad += num_in_string;

        for (uint16_t i=digit ; i> 0 ; i--){
            right_aligned += pad[pad.length()-i];
        }
    }

    return right_aligned;
}



