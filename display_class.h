#ifndef _LCDDISPLAY_CLASS_H_
#define _LCDDISPLAY_CLASS_H_

#include <rgb_lcd.h>
#include "eh900_class.h"

constexpr uint16_t POSITION_BAR_GRAPH       = 10;
constexpr uint16_t POSITION_SENSOR_LENGTH   = 1;
constexpr uint16_t POSITION_TIMER_SET       = 5;
constexpr uint16_t POSITION_TIMER_COUNT     = 2;
constexpr uint16_t POSITION_LEVEL           = 10;
constexpr uint16_t POSITION_MODE            = 0;

class Eh_display : public rgb_lcd {

    public:
        // コンストラクタ  

        Eh_display();
        
        //  LCDのイニシャライズ  液面計オブジェクトを渡す
        //      error が0以外ならエラーを表示する
        void init(eh900* pModel,uint16_t error);

        //  液面レベルの表示    数値＋バーグラフ
        void showLevel(void);

        //  モードの表示 100ms程度で周期的に呼ぶことでスムースな表示になる
        void showMode(void);

        void showTimer(void);


    private:
        eh900* pMeter;

};

//  指定桁数の数字（ストリング）を右寄せで表示
String right_align(String num_in_string, uint16_t digit);

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

#endif // _DISPLAY_CLASS_H_